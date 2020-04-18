/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
struct BufferedPercent
{
	f32 duration;
	f32 current;
};

f32 move_towards(f32 target, f32 elapsedTime, BufferedPercent & buffer)
{
	if (target < buffer.current)
	{
		buffer.current -= elapsedTime / buffer.duration;
		buffer.current = math::max(target, buffer.current);
	}
	else if (target > buffer.current)
	{
		buffer.current += elapsedTime / buffer.duration;
		buffer.current = math::min(target, buffer.current);
	}

	return buffer.current;
}


namespace CharacterAnimations
{
	enum AnimationType { IDLE, CROUCH, WALK, RUN, JUMP, FALL, ANIMATION_COUNT };
}

struct CharacterController3rdPerson
{
	// References
	Transform3D * transform;

	// Properties
	f32 walkSpeed = 3;
	f32 runSpeed = 6;
	f32 jumpSpeed = 6;

	f32 gravity = -9.81;

	f32 collisionRadius = 0.25f;

	// State
	f32 	zSpeed;
	v3 		forward;

	v3 hitRayPosition;
	v3 hitRayNormal;

	// Animations
	AnimatedSkeleton 	skeleton;

	Animation animations[CharacterAnimations::ANIMATION_COUNT];

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

	f32 currentSpeed = 0;
	/* Note(Leo): As in change of velocity per time unit. These probably
	should be kept at values relative to max speed */ 
	f32 acceleration = 1.0f * runSpeed;
	f32 deceleration = -2.0f * runSpeed;

	f32 rotationSpeed = to_radians(360);
};

internal CharacterController3rdPerson
make_character(Transform3D * transform)
{
	CharacterController3rdPerson result = {};
	result.transform 	= transform;
	result.zSpeed 		= 0.0f;
	result.forward 		= get_forward(*transform);
	return result;
}

internal v3
process_player_input(game::Input const * input, Camera const * camera)
{
	v3 viewForward 	= get_forward(camera);
	viewForward.z 	= 0;
	viewForward 	= viewForward.normalized();
	v3 viewRight 	= vector::cross(viewForward, world::up);
	v3 result 		= viewRight * input->move.x + viewForward * input->move.y;

	logDebug(0) << "input move: " <<  input->move;

	return result;
}

void
update_character( 	CharacterController3rdPerson & 	character,
					game::Input * 					input,
					Camera * 						worldCamera,
					CollisionSystem3D *				collisionSystem,
					platform::Graphics * 			graphics)
{
	v3 inputVector = process_player_input(input, worldCamera);

	// f32 inputMagnitude = inputVector.magnitude();
	// logDebug() << inputVector << ", " << inputMagnitude;
	// Assert(inputMagnitude == inputMagnitude);


	v3 forward = get_forward(*character.transform);
	v3 right = get_right(*character.transform);

	// Note(Leo): these are in character space
	f32 forwardInput = vector::dot(inputVector, forward);
	f32 rightInput = vector::dot(inputVector, right);

	v4 gizmoColor = {0,0,0,1};

	constexpr f32 walkMinInput 	= 0.05f;
	constexpr f32 walkMaxInput 	= 0.5f;
	constexpr f32 runMinInput 	= 0.6f;
	constexpr f32 runMaxInput 	= 1.0001f; // account for epsilon

	constexpr f32 idleToWalkRange = walkMaxInput - walkMinInput; 
	constexpr f32 walkToRunRange 	= runMaxInput - runMinInput;

	using namespace CharacterAnimations;

	f32 weights [ANIMATION_COUNT] = {};

	auto override_weight = [&weights](AnimationType animation, f32 value)
	{
		Assert(-0.00001f <= value && value <= 1.00001f);
		for (int i = 0; i < ANIMATION_COUNT; ++i)
		{
			f32 weightBefore = weights[i];
			weights[i] *= 1 - value;

			Assert(-0.00001f <= weights[i] && weights[i] <= 1.00001f);
			if (i == animation)
			{
				weights[i] += value;
			}
		}
	};

	Animation const * animations [ANIMATION_COUNT];
	animations[IDLE] 	= &character.animations[IDLE];
	animations[CROUCH] 	= &character.animations[CROUCH];
	animations[WALK] 	= &character.animations[WALK];
	animations[RUN] 	= &character.animations[RUN];
	animations[JUMP] 	= &character.animations[JUMP];
	animations[FALL] 	= &character.animations[FALL];

	f32 crouchPercent = move_towards(is_pressed(input->B), input->elapsedTime, character.crouchPercent);

	constexpr f32 crouchOverridePowerForAnimation = 0.6f;
	constexpr f32 crouchOverridePowerForSpeed = 0.8f;

	f32 speed = 0;

	// -------------------------------------------------
	// Update speeds
	// -------------------------------------------------

	if (forwardInput < walkMinInput)
	{
		speed 			= 0;
		gizmoColor 		= {0, 0, 1, 1};
	}
	else if (forwardInput < walkMaxInput)
	{
		f32 t = (forwardInput - walkMinInput) / idleToWalkRange;
		speed 			= interpolate(0, character.walkSpeed, t);
		f32 crouchValue = crouchPercent;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		gizmoColor 		= {1, 0, 0, 1};
	}
	else if (forwardInput < runMinInput)
	{
		speed 			= character.walkSpeed;
		f32 crouchValue = crouchPercent;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		gizmoColor 		= {1, 1, 0, 1};
	}
	else
	{
		f32 t = (forwardInput - runMinInput) / walkToRunRange;
		speed 			= interpolate (character.walkSpeed, character.runSpeed, t);
		f32 crouchValue = crouchPercent;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		gizmoColor = {0, 1, 0, 1};
	}

	if (speed > character.currentSpeed)
	{
		character.currentSpeed += character.acceleration * input->elapsedTime;
		character.currentSpeed = math::min(character.currentSpeed, speed);
	}
	else if (speed < character.currentSpeed)
	{
		character.currentSpeed += character.deceleration * input->elapsedTime;
		character.currentSpeed = math::max(character.currentSpeed, speed);
	}

	speed = character.currentSpeed;


	// ----------------------------------------------
	// Update animation weigthes
	// ----------------------------------------------

	if (speed < 0.00001f)
	{
		weights[IDLE] 	= 1 * (1 - crouchPercent);
		weights[CROUCH] = 1 * crouchPercent;
	}
	else if (speed < character.walkSpeed)
	{
		f32 t = speed / character.walkSpeed;

		weights[IDLE] 	= (1 - t) * (1 - crouchPercent);
		weights[CROUCH] = (1 - t) * crouchPercent;
		weights[WALK] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}
	else
	{
		f32 t = (speed - character.walkSpeed) / (character.runSpeed - character.walkSpeed);

		weights[WALK] 	= 1 - t;
		weights[RUN] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}

	// ------------------------------------------------------------------------

	bool32 falling = character.zSpeed < - 1;
	move_towards(falling ? (1 - character.grounded.current) : 0, input->elapsedTime, character.fallPercent);
	override_weight(FALL, math::smooth(character.fallPercent.current));

	s32 goingUp = character.zSpeed > 0.1;
	move_towards(goingUp, input->elapsedTime, character.jumpPercent);
	override_weight(JUMP, math::smooth(character.jumpPercent.current));

	// ------------------------------------------------------------------------

	// Note(Leo): 0 when landing does not affect, 1 when it affects at max;
	f32 landingValue = 0;

	if (character.isLanding)
	{
		if (character.landingTimer > 0)
		{
			using namespace math;

			character.landingTimer -= input->elapsedTime;

			// Note(Leo): this is basically a relative time passed
			landingValue = character.landingDepth * (1 - smooth((character.landingDuration - character.landingTimer) / character.landingDuration));

			f32 crouchWeight = smooth(1.0f - absolute((character.landingDuration - (2 * character.landingTimer)) / character.landingDuration));
			crouchWeight *= character.landingDepth;
			override_weight(CROUCH, crouchWeight);
		}
		else
		{
			character.isLanding = false;
		}
	}

	// speed *= 1 - landingValue;



	if (speed > 0)
	{
		v3 direction = forward;//inputMagnitude > 0.00001f ? inputVector.normalized() : get_forward(*character.transform);
		f32 distance = input->elapsedTime * speed;

		constexpr int rayCount = 5;
		v3 up 	= get_up(*character.transform);

		v3 rayStartPositions [rayCount];
		f32 sineStep 	= 2.0f / (rayCount - 1);

		rayStartPositions[0] = vector::rotate(direction * character.collisionRadius, up, pi / 2.0f);
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
			v3 start = rayStartPositions[i] + up * 0.25f + character.transform->position;

			bool32 hit = raycast_3d(collisionSystem, start, direction, distance, &raycastResult);
			rayHit = rayHit || hit;

			float4 lineColor = hit ? float4{0,1,0,1} : float4{1,0,0,1};
			// platformApi->draw_line(graphics, start, start + direction, 5.0f, lineColor);

		}

		if (rayHit == false)
		{
			character.transform->position += direction * distance;
		}
		else
		{
			character.hitRayPosition = character.transform->position + up * 0.25f;
			character.hitRayNormal = raycastResult.hitNormal;

			// TOdo(Leo): Make so that we only reduce the amount of normal's part
			// auto projectionOnNormal = vector::project(direction * distance, raycastResult.hitNormal);
			// character.transform->position += direction * distance - projectionOnNormal;
		}
	}

	// Note(Leo): This if is not super important but maybe we dodge a few weird cases. 
	if (rightInput != 0)
	{
		/* Note(Leo): input is inverted, because negative input means left,
		but in our right handed coordinate system, negative rotation means right */
		quaternion rotation = quaternion::axis_angle(v3::up, -1 * rightInput * character.rotationSpeed * input->elapsedTime);
		character.transform->rotation = character.transform->rotation * rotation;
	}

	f32 groundHeight = get_terrain_height(collisionSystem, vector::convert_to<v2>(character.transform->position));
	bool32 grounded = character.transform->position.z < (0.1f + groundHeight);

	bool32 startLanding = character.wasGroundedLastFrame == false && grounded == true;
	character.wasGroundedLastFrame = grounded;

	move_towards(grounded, input->elapsedTime, character.grounded);

	grounded = character.grounded.current > 0.5f;

	// This is jump
	if (grounded && is_clicked(input->X))
	{
		character.jumpTimer = character.jumpDuration;
		character.goingToJump = true;
	}

	if (character.goingToJump)
	{
		if(character.jumpTimer > 0)
		{
			using namespace math;

			character.jumpTimer -= input->elapsedTime;

			// Note(Leo): Sorry future me, math is fun :D If this is confusing try plotting it in desmos.com
			f32 crouchWeight = smooth(1.0f - absolute((-2 * character.jumpTimer + character.jumpDuration) / character.jumpDuration));
			override_weight(CROUCH, crouchWeight);
		}
		else
		{
			character.goingToJump = false;
			character.zSpeed = character.jumpSpeed;
		}
	}

	if (startLanding)
	{
		character.landingDepth = (math::absolute(character.zSpeed) - character.minLandingDepthZSpeed)
									/ (character.maxLandingDepthZSpeed - character.minLandingDepthZSpeed);
		character.landingDepth = math::clamp(character.landingDepth, 0.0f, 1.0f);

		character.landingDuration = character.maxlandingDuration * character.landingDepth;
		character.landingTimer = character.landingDuration;
		character.isLanding = true;
	}

	character.transform->position.z += character.zSpeed * input->elapsedTime;

	if (character.transform->position.z > groundHeight)
	{	
		character.zSpeed += character.gravity * input->elapsedTime;
	}
	else
	{
		character.zSpeed = 0;
        character.transform->position.z = groundHeight;
	}

	// ----------------------------------------------------------------------------------

	// Note(Leo): We do this late here because we update animation weights later too.
	update_skeleton_animator(character.skeleton, animations, weights, ANIMATION_COUNT, input->elapsedTime);

	// ----------------------------------------------------------------------------------

	// Note(Leo): Draw move action gizmo after movement, so it does not lage one frame behind
	v3 gizmoPosition = character.transform->position + v3{0,0,2};
	m44 gizmoTransform = make_transform_matrix(	gizmoPosition, 0.3f);
	debug::draw_diamond(graphics, gizmoTransform, gizmoColor, platformApi);

	debug::draw_line(gizmoPosition, gizmoPosition + rightInput * right, {0.8, 0.2, 0.3, 1.0});
	debug::draw_line(gizmoPosition, gizmoPosition + forwardInput * forward, {0.2, 0.8, 0.3, 1.0});

	debug::draw_line(gizmoPosition, gizmoPosition + input->move.x * get_right(worldCamera), {0.8, 0.8, 0.2});
	debug::draw_line(gizmoPosition, gizmoPosition + input->move.y * get_forward(worldCamera), {0.2, 0.3, 0.8});


	if (grounded)
	{
		gizmoTransform = make_transform_matrix(character.transform->position + v3{0,0,2}, 0.1);
		debug::draw_diamond(graphics, gizmoTransform, gizmoColor, platformApi);
	}

} // update_character()