struct OtherGirlController
{
	Transform3D * transform;
	Transform3D * targetTransform;

	f32 zSpeed;
	f32 currentSpeed;


	f32 targetDistanceThreshold = 2;
	f32 walkSpeed = 2.5;
	f32 runSpeed = 5;
	f32 rotationSpeed = to_radians(360);
	f32 jumpSpeed = 5;

	f32 acceleration = 1 * runSpeed;
	f32 deceleration = -2 * runSpeed;


	BufferedPercent fallPercent = { .duration = 0.2f };
	BufferedPercent jumpPercent = { .duration = 0.2f };
	BufferedPercent crouchPercent = { .duration = 0.2f };
	BufferedPercent grounded = { .duration = 0.5f };

	f32 	jumpTimer;
	f32 	jumpDuration = 0.2f;
	bool8 	goingToJump;
	bool8	wasGroundedLastFrame;
	bool8 	isLanding;

	f32 landingTimer;
	f32 landingDuration;
	f32 maxlandingDuration = 0.8f;
	f32 landingDepth;
	f32 minLandingDepthZSpeed = 2.0f;
	f32 maxLandingDepthZSpeed = 8.0f;

	f32 collisionRadius = 0.25f;
	v3 hitRayPosition;
	v3 hitRayNormal;

	AnimatedSkeleton 	skeleton;
	Animation const * 	animations [CharacterAnimations::ANIMATION_COUNT];
	f32 				animationWeights [CharacterAnimations::ANIMATION_COUNT];
};

void update_other_girl(	OtherGirlController & controller,
						CollisionSystem3D & collisionSystem,
						f32 elapsedTime)
{
	v3 toTarget 			= controller.targetTransform->position - controller.transform->position;
	toTarget.z = 0;
	float distanceToTarget 	= toTarget.magnitude();

	distanceToTarget = math::clamp(distanceToTarget - controller.targetDistanceThreshold, 0.0f, 1.0f);

	v3 inputVector = distanceToTarget < 0.00001f
					? v3 {0, 0, 0}
					: toTarget.normalized() * math::min(distanceToTarget, 1.0f);


	v3 forward 	= get_forward(*controller.transform);
	v3 right 	= get_right(*controller.transform);

	// Note(Leo): these are in character space
	f32 forwardInput 	= vector::dot(inputVector, forward);
	f32 rightInput 		= vector::dot(inputVector, right);

	v3 gizmoPosition = controller.transform->position + v3{0,0,2};
	debug::draw_line(gizmoPosition, gizmoPosition + right * rightInput, colors::mutedRed);
	debug::draw_line(gizmoPosition, gizmoPosition + forward * forwardInput, colors::mutedGreen);

	// -------------------------------------------------

	constexpr f32 walkMinInput 	= 0.05f;
	constexpr f32 walkMaxInput 	= 0.5f;
	constexpr f32 runMinInput 	= 0.6f;
	constexpr f32 runMaxInput 	= 1.0001f; // account for epsilon

	constexpr f32 idleToWalkRange 	= walkMaxInput - walkMinInput; 
	constexpr f32 walkToRunRange 	= runMaxInput - runMinInput;

	using namespace CharacterAnimations;

	f32 * weights = controller.animationWeights;
	for (f32 & weight : controller.animationWeights)
	{
		weight = 0;
	}

	auto override_weight = [&weights](AnimationType animation, f32 value)
	{
		Assert(-0.00001f <= value && value <= 1.00001f);
		for (int i = 0; i < ANIMATION_COUNT; ++i)
		{
			f32 weightBefore = weights[i];
			weights[i] *= 1 - value;

			if (weights[i] < -0.00001f || weights[i] > 1.00001f)
			{
				float a = weights[i];
			}

			Assert(-0.00001f <= weights[i] && weights[i] <= 1.00001f);
			if (i == animation)
			{
				weights[i] += value;
			}
		}
	};

	f32 speed = 0;
	f32 crouchPercent = 0;
	constexpr f32 crouchOverridePowerForAnimation = 0.5f;
	constexpr f32 crouchOverridePowerForSpeed = 0.8f;

	// -------------------------------------------------
	// Update speeds
	// -------------------------------------------------

	if (forwardInput < walkMinInput)
	{
		speed 		= 0;
	}
	else if (forwardInput < walkMaxInput)
	{
		f32 t 		= (forwardInput - walkMinInput) / idleToWalkRange;
		speed 		= interpolate(0, controller.walkSpeed, t);
		speed 		*= (1 - crouchPercent * crouchOverridePowerForSpeed);
	}
	else if (forwardInput < runMinInput)
	{
		speed 		= controller.walkSpeed;
		speed 		*= (1 - crouchPercent * crouchOverridePowerForSpeed);
	}
	else
	{
		f32 t 		= (forwardInput - runMinInput) / walkToRunRange;
		speed 		= interpolate (controller.walkSpeed, controller.runSpeed, t);
		speed 		*= (1 - crouchPercent * crouchOverridePowerForSpeed);
	}

	if (speed > controller.currentSpeed)
	{
		controller.currentSpeed += controller.acceleration * elapsedTime;
		controller.currentSpeed = math::min(controller.currentSpeed, speed);
	}
	else if (speed < controller.currentSpeed)
	{
		controller.currentSpeed += controller.deceleration * elapsedTime;
		controller.currentSpeed = math::max(controller.currentSpeed, speed);
	}

	speed = controller.currentSpeed;

	// Note(Leo): 0 when landing does not affect, 1 when it affects at max;
	f32 crouchWeightFromLanding = 0;

	if (controller.isLanding)
	{
		if (controller.landingTimer > 0)
		{
			using namespace math;

			controller.landingTimer -= elapsedTime;

			// Note(Leo): this is basically a relative time passed
			f32 landingValue = controller.landingDepth * (1 - smooth((controller.landingDuration - controller.landingTimer) / controller.landingDuration));

			// f32 crouchWeight 
			crouchWeightFromLanding = smooth(1.0f - absolute((controller.landingDuration - (2 * controller.landingTimer)) / controller.landingDuration));
			crouchWeightFromLanding *= controller.landingDepth;
		}
		else
		{
			controller.isLanding = false;
		}
	}

	if (speed > 0)
	{
		v3 direction = forward;//inputMagnitude > 0.00001f ? inputVector.normalized() : get_forward(*controller.transform);
		f32 distance = elapsedTime * speed;

		constexpr int rayCount = 5;
		v3 up 	= get_up(*controller.transform);

		v3 rayStartPositions [rayCount];
		f32 sineStep 	= 2.0f / (rayCount - 1);

		rayStartPositions[0] = vector::rotate(direction * controller.collisionRadius, up, pi / 2.0f);
		for (int i = 1; i < rayCount; ++i)
		{
			f32 sine 		= -1.0f + (i - 1) * sineStep;
			f32 angle 	= -arc_cosine(sine);
			rayStartPositions[i] = vector::rotate(rayStartPositions[0], up, angle);
		}

	
		bool32 rayHit = false;
		RaycastResult raycastResult;
		for (int i  = 0; i < rayCount; ++i)
		{
			v3 start = rayStartPositions[i] + up * 0.25f + controller.transform->position;

			bool32 hit = raycast_3d(&collisionSystem, start, direction, distance, &raycastResult);
			rayHit = rayHit || hit;

			float4 lineColor = hit ? float4{0,1,0,1} : float4{1,0,0,1};
			// platformApi->draw_line(graphics, start, start + direction, 5.0f, lineColor);

		}

		if (rayHit == false)
		{
			controller.transform->position += direction * distance;
		}
		else
		{
			controller.hitRayPosition = controller.transform->position + up * 0.25f;
			controller.hitRayNormal = raycastResult.hitNormal;

			// TOdo(Leo): Make so that we only reduce the amount of normal's part
			// auto projectionOnNormal = vector::project(direction * distance, raycastResult.hitNormal);
			// controller.transform->position += direction * distance - projectionOnNormal;
		}
	}

	// Note(Leo): This if is not super important but maybe we dodge a few weird cases. 
	if (rightInput != 0)
	{
		/* Note(Leo): input is inverted, because negative input means left,
		but in our right handed coordinate system, negative rotation means right */
		quaternion rotation 			= quaternion::axis_angle(v3::up, -1 * rightInput * controller.rotationSpeed * elapsedTime);
		controller.transform->rotation 	= controller.transform->rotation * rotation;
	}

	// This is jump
	// if (grounded && is_clicked(input->X))
	// {
	// 	controller.jumpTimer = controller.jumpDuration;
	// 	controller.goingToJump = true;
	// }

	float crouchWeightFromJumping = 0;

	if (controller.goingToJump)
	{
		if(controller.jumpTimer > 0)
		{
			using namespace math;

			controller.jumpTimer -= elapsedTime;

			// Note(Leo): Sorry future me, math is fun :D If this is confusing try plotting it in desmos.com
			crouchWeightFromJumping = smooth(1.0f - absolute((-2 * controller.jumpTimer + controller.jumpDuration) / controller.jumpDuration));
		}
		else
		{
			controller.goingToJump = false;
			controller.zSpeed = controller.jumpSpeed;
		}
	}

	// --------------------------------------------------------------------

	f32 groundHeight = get_terrain_height(&collisionSystem, vector::convert_to<v2>(controller.transform->position));
	bool32 grounded = controller.transform->position.z < (0.1f + groundHeight);

	bool32 startLanding = controller.wasGroundedLastFrame == false && grounded == true;
	controller.wasGroundedLastFrame = grounded;

	move_towards(grounded, elapsedTime, controller.grounded);

	grounded = controller.grounded.current > 0.5f;

	if (startLanding)
	{
		controller.landingDepth = (math::absolute(controller.zSpeed) - controller.minLandingDepthZSpeed)
									/ (controller.maxLandingDepthZSpeed - controller.minLandingDepthZSpeed);
		controller.landingDepth = math::clamp(controller.landingDepth, 0.0f, 1.0f);

		controller.landingDuration = controller.maxlandingDuration * controller.landingDepth;
		controller.landingTimer = controller.landingDuration;
		controller.isLanding = true;
	}

	controller.transform->position.z += controller.zSpeed * elapsedTime;

	if (controller.transform->position.z > groundHeight)
	{	
		controller.zSpeed += physics::gravityAcceleration * elapsedTime;
	}
	else
	{
		controller.zSpeed = 0;
        controller.transform->position.z = groundHeight;
	}

	bool32 falling = controller.zSpeed < - 1;
	move_towards(falling ? (1 - controller.grounded.current) : 0, elapsedTime, controller.fallPercent);

	s32 goingUp = controller.zSpeed > 0.1;
	move_towards(goingUp, elapsedTime, controller.jumpPercent);

	// ----------------------------------------------
	// Update animation weigthes
	// ----------------------------------------------

	if (speed < 0.00001f)
	{
		weights[IDLE] 	= 1 * (1 - crouchPercent);
		weights[CROUCH] = 1 * crouchPercent;
	}
	else if (speed < controller.walkSpeed)
	{
		f32 t = speed / controller.walkSpeed;

		weights[IDLE] 	= (1 - t) * (1 - crouchPercent);
		weights[CROUCH] = (1 - t) * crouchPercent;
		weights[WALK] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}
	else
	{
		f32 t = (speed - controller.walkSpeed) / (controller.runSpeed - controller.walkSpeed);

		weights[WALK] 	= 1 - t;
		weights[RUN] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}

	override_weight(FALL, math::smooth(controller.fallPercent.current));
	override_weight(JUMP, math::smooth(controller.jumpPercent.current));
	override_weight(CROUCH, math::max(crouchWeightFromLanding, crouchWeightFromJumping));

}