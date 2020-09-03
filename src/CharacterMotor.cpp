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
		buffer.current = max_f32(target, buffer.current);
	}
	else if (target > buffer.current)
	{
		buffer.current += elapsedTime / buffer.duration;
		buffer.current = min_f32(target, buffer.current);
	}

	return buffer.current;
}

namespace CharacterAnimations
{
	enum AnimationType { IDLE, CROUCH, WALK, RUN, JUMP, FALL, ANIMATION_COUNT };
}

struct CharacterMotor
{
	Transform3D * 	transform;

	Animation const * 	animations [CharacterAnimations::ANIMATION_COUNT];
	f32 				animationWeights [CharacterAnimations::ANIMATION_COUNT];

	BufferedPercent fallPercent = { .duration = 0.2f };
	BufferedPercent jumpPercent = { .duration = 0.2f };
	BufferedPercent crouchPercent = { .duration = 0.2f };
	BufferedPercent grounded = { .duration = 0.5f };

	// Properties
	f32 walkSpeed 	= 3;
	f32 runSpeed 	= 6;
	f32 jumpSpeed 	= 6;

	f32 rotationSpeed = to_radians(360);

	f32 currentSpeed = 0;
	/* Note(Leo): As in change of velocity per time unit. These probably
	should be kept at values relative to max speed */ 
	f32 acceleration = 1.0f * runSpeed;
	f32 deceleration = -2.0f * runSpeed;

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

	f32 	zSpeed;
};

struct CharacterInput
{
	v3 inputVector;
	bool32 jumpInput;
	bool32 crouchInput;
};

void
update_character_motor( CharacterMotor & 	motor,
						CharacterInput input,
						CollisionSystem3D &	collisionSystem,
						f32 elapsedTime,
						s32 debugLevel)
{
	v3 inputVector 		= input.inputVector;
	bool32 jumpInput 	= input.jumpInput;
	bool32 crouchInput 	= input.crouchInput;

	// -------------------------------------------------
	
	v3 forward 	= get_forward(*motor.transform);
	v3 right 	= get_right(*motor.transform);

	// Note(Leo): these are in motor's local space
	f32 forwardInput	= dot_v3(inputVector, forward);

	f32 rightInput 		= 0;
	if (square_magnitude_v3(inputVector) > v3_sqr_epsilon)
	{
		f32 angle = v2_signed_angle(inputVector.xy, forward.xy);
		rightInput = clamp_f32(angle, -1.0f, 1.0f);
	}

	FS_DEBUG(debugLevel, debug_draw_axes(translation_matrix({0,0,2}) * transform_matrix(*motor.transform), 0.3f));

	v3 points [] = {motor.transform->position + v3{0,0,1.9},
					motor.transform->position + v3{0,0,1.9} + right * rightInput};
	FS_DEBUG(debugLevel, debug_draw_lines(2, points, color_muted_purple));


	// -------------------------------------------------

	constexpr f32 walkMinInput 	= 0.00f;
	constexpr f32 walkMaxInput 	= 0.5f;
	constexpr f32 runMinInput 	= 0.6f;
	constexpr f32 runMaxInput 	= 1.0001f; // account for epsilon

	constexpr f32 idleToWalkRange = walkMaxInput - walkMinInput; 
	constexpr f32 walkToRunRange 	= runMaxInput - runMinInput;

	using namespace CharacterAnimations;

	f32 * weights = motor.animationWeights;
	for (s32 weightIndex = 0; weightIndex < ANIMATION_COUNT; ++weightIndex)
	{
		weights[weightIndex] = 0;
	}

	auto override_weight = [&weights](AnimationType animation, f32 value)
	{
		if (-0.00001f > value || value > 1.00001f)
		{
			logDebug() << "Bad value";
		}

		AssertMsg(-0.00001f <= value && value <= 1.00001f, CStringBuilder("Input weight is: ") + value);
		for (int i = 0; i < ANIMATION_COUNT; ++i)
		{
			weights[i] *= 1 - value;

			AssertMsg(-0.00001f <= weights[i] && weights[i] <= 1.00001f, CStringBuilder("Weight is: ") + weights[i]);
			if (i == animation)
			{
				weights[i] += value;
			}
		}
	};

	f32 crouchPercent = move_towards(crouchInput, elapsedTime, motor.crouchPercent);

	constexpr f32 crouchOverridePowerForAnimation = 0.5f;
	constexpr f32 crouchOverridePowerForSpeed = 0.8f;

	f32 speed = 0;

	// -------------------------------------------------
	// Update speeds
	// -------------------------------------------------

	if (forwardInput < walkMaxInput)
	{
		f32 t 			= (forwardInput - walkMinInput) / idleToWalkRange;
		speed 			= interpolate(0, motor.walkSpeed, t);
		speed 			*= (1 - crouchPercent * crouchOverridePowerForSpeed);
	}
	else if (forwardInput < runMaxInput)
	{
		f32 t 			= (forwardInput - runMinInput) / walkToRunRange;
		speed 			= interpolate (motor.walkSpeed, motor.runSpeed, t);
		speed 			*= (1 - crouchPercent * crouchOverridePowerForSpeed);
	}

	if (speed > motor.currentSpeed)
	{
		motor.currentSpeed += motor.acceleration * elapsedTime;
		motor.currentSpeed = min_f32(motor.currentSpeed, speed);
	}
	else if (speed < motor.currentSpeed)
	{
		motor.currentSpeed += motor.deceleration * elapsedTime;
		motor.currentSpeed = max_f32(motor.currentSpeed, speed);
	}

	speed = motor.currentSpeed;

	// Note(Leo): 0 when landing does not affect, 1 when it affects at max;
	f32 crouchWeightFromLanding = 0;

	if (motor.isLanding)
	{
		if (motor.landingTimer > 0)
		{
			motor.landingTimer = max_f32(motor.landingTimer -elapsedTime, 0.0f);

			// Note(Leo): this is basically a relative time passed, to be used in other evaluations too
			// landingValue = motor.landingDepth * (1 - mathfun_smooth_f32((motor.landingDuration - motor.landingTimer) / motor.landingDuration));

			crouchWeightFromLanding = mathfun_smooth_f32(1.0f - abs_f32((motor.landingDuration - (2 * motor.landingTimer)) / motor.landingDuration));
			crouchWeightFromLanding *= motor.landingDepth;
		}
		else
		{
			motor.isLanding = false;
		}
	}

	if (speed > 0)
	{
		v3 direction = forward;
		f32 distance = elapsedTime * speed;

		constexpr int rayCount = 5;
		v3 up 	= get_up(*motor.transform);

		v3 rayStartPositions [rayCount];
		f32 sineStep 	= 2.0f / (rayCount - 1);

		f32 skinwidth = 0.01f;
		rayStartPositions[0] = rotate_v3(direction * (motor.collisionRadius - skinwidth), up, π / 2.0f);
		for (int i = 1; i < rayCount; ++i)
		{
			f32 sine 		= -1.0f + (i - 1) * sineStep;
			f32 angle 	= -arc_cosine(sine);
			rayStartPositions[i] = rotate_v3(rayStartPositions[0], up, angle);
		}

		RaycastResult rayHitResults[rayCount];
		s32 rayHitCount = 0;

		v3 rayDebugHitPoints [2 * rayCount];
		s32 rayDebugHitCount = 0;

		v3 rayDebugMissPoints [2 * rayCount];
		s32 rayDebugMissCount = 0;


		for (int i  = 0; i < rayCount; ++i)
		{
			v3 start = rayStartPositions[i] + up * 0.25f + motor.transform->position;

			RaycastResult currentResult;
			bool32 hit = raycast_3d(&collisionSystem, start, direction, (distance * 2 + skinwidth), &currentResult);

			if (hit)
			{
				rayHitResults[rayHitCount] = currentResult;
				++rayHitCount;

				rayDebugHitPoints[rayDebugHitCount] = start;
				rayDebugHitPoints[rayDebugHitCount + 1] = start + direction;
				rayDebugHitCount += 2;

				FS_DEBUG(debugLevel, debug_draw_vector(currentResult.hitPosition, currentResult.hitNormal, colour_bright_yellow));
			}
			else
			{
				rayDebugMissPoints[rayDebugMissCount] = start;
				rayDebugMissPoints[rayDebugMissCount + 1] = start + direction;
				rayDebugMissCount += 2;
			}

		if (rayDebugHitCount > 0)
			FS_DEBUG(debugLevel, debug_draw_lines(rayDebugHitCount, rayDebugHitPoints, colour_bright_red));
		if (rayDebugMissCount > 0)
			FS_DEBUG(debugLevel, debug_draw_lines(rayDebugMissCount, rayDebugMissPoints, colour_muted_green));
		}

		if (rayHitCount == 0)
		{
			motor.transform->position += direction * distance;
		}
		else
		{
			v3 movement = direction * distance;

			for (s32 i = 0; i < rayHitCount; ++i)
			{
				f32 projectionLength 	= min_f32(0.0f, dot_v3(movement, rayHitResults[i].hitNormal));
				v3 projection 			= rayHitResults[i].hitNormal * projectionLength;
				movement 				-= projection;
			}

			motor.transform->position += movement;
		}
	}

	// Note(Leo): This if is not super important but maybe we dodge a few weird cases. 
	if (rightInput != 0)
	{
		/* Note(Leo): input is inverted, because negative input means left,
		but in our right handed coordinate system, negative rotation means right */
		quaternion rotation = axis_angle_quaternion(up_v3, -rightInput * motor.rotationSpeed * elapsedTime);
		motor.transform->rotation = motor.transform->rotation * rotation;
	}

	f32 crouchWeightFromJumping = 0;

	if (motor.goingToJump)
	{
		if(motor.jumpTimer > 0)
		{
			motor.jumpTimer -= elapsedTime;
			motor.jumpTimer = max_f32(motor.jumpTimer - elapsedTime, 0.0f);

			// Note(Leo): Sorry future me, math is fun :D If this is confusing try plotting it in desmos.com
			crouchWeightFromJumping = mathfun_smooth_f32(1.0f - abs_f32((-2 * motor.jumpTimer + motor.jumpDuration) / motor.jumpDuration));
		}
		else
		{
			motor.goingToJump = false;
			motor.zSpeed = motor.jumpSpeed;
		}
	}

	// ----------------------------------------------------------------------------------


	f32 groundThreshold = 0.01f;
	f32 groundHeight 	= get_terrain_height(collisionSystem, motor.transform->position.xy);
	bool32 grounded 	= motor.transform->position.z < (groundThreshold + groundHeight);

	// CHECK COLLISION WITH OTHER COLLIDERS TOO
	{
		f32 groundRaySkinWidth 	= 1.0f;
		v3 groundRayStart 		= motor.transform->position + v3{0,0,groundRaySkinWidth};
		v3 groundRayDirection 	= -up_v3;
		f32 groundRayLength 	= groundRaySkinWidth + max_f32(0.1f, abs_f32(motor.zSpeed));

		RaycastResult rayResult;
		if (raycast_3d(&collisionSystem, groundRayStart, groundRayDirection, groundRayLength, &rayResult))
		{
			// Notice: Only store more dramatic state
			groundHeight 	= max_f32(groundHeight, rayResult.hitPosition.z);
			grounded 		= grounded || motor.transform->position.z < (groundThreshold + groundHeight);

			FS_DEBUG(debugLevel, debug_draw_line(motor.transform->position, rayResult.hitPosition, colour_dark_red));
			FS_DEBUG(debugLevel, debug_draw_cross_xy(motor.transform->position, 0.3, colour_bright_yellow));
			FS_DEBUG(debugLevel, debug_draw_cross_xy(rayResult.hitPosition, 0.3, colour_bright_purple));
		}
		else
		{
			FS_DEBUG(debugLevel, debug_draw_line(motor.transform->position, motor.transform->position - v3{0,0,1}, colour_bright_green));
		}
	}

	bool32 startLanding 		= motor.wasGroundedLastFrame == false && grounded == true;
	motor.wasGroundedLastFrame 	= grounded;

	move_towards(grounded, elapsedTime, motor.grounded);

	grounded = motor.grounded.current > 0.5f;

	// This is jump
	if (grounded && jumpInput)
	{
		motor.jumpTimer = motor.jumpDuration;
		motor.goingToJump = true;
	}

	if (startLanding)
	{
		motor.landingDepth = (abs_f32(motor.zSpeed) - motor.minLandingDepthZSpeed)
									/ (motor.maxLandingDepthZSpeed - motor.minLandingDepthZSpeed);
		motor.landingDepth = clamp_f32(motor.landingDepth, 0.0f, 1.0f);

		motor.landingDuration = motor.maxlandingDuration * motor.landingDepth;
		motor.landingTimer = motor.landingDuration;
		motor.isLanding = true;
	}

	motor.transform->position.z += motor.zSpeed * elapsedTime;

	if ((motor.transform->position.z > groundHeight))
	{	
		motor.zSpeed += physics_gravity_acceleration * elapsedTime;
	}
	else
	{
		motor.zSpeed = max_f32(0.0f, motor.zSpeed);
        motor.transform->position.z = groundHeight;
	}

	// ----------------------------------------------
	// Update animation weigthes
	// ----------------------------------------------

	bool32 falling = motor.zSpeed < - 1;
	move_towards(falling ? (1 - motor.grounded.current) : 0, elapsedTime, motor.fallPercent);

	s32 goingUp = motor.zSpeed > 0.1;
	move_towards(goingUp, elapsedTime, motor.jumpPercent);

	if (speed < 0.00001f)
	{
		weights[IDLE] 	= 1 * (1 - crouchPercent);
		weights[CROUCH] = 1 * crouchPercent;
	}
	else if (speed < motor.walkSpeed)
	{
		f32 t = speed / motor.walkSpeed;

		weights[IDLE] 	= (1 - t) * (1 - crouchPercent);
		weights[CROUCH] = (1 - t) * crouchPercent;
		weights[WALK] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}
	else
	{
		f32 t = (speed - motor.walkSpeed) / (motor.runSpeed - motor.walkSpeed);

		weights[WALK] 	= 1 - t;
		weights[RUN] 	= t;

		f32 crouchValue = crouchPercent;
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);
	}

	override_weight(FALL, mathfun_smooth_f32(motor.fallPercent.current));
	override_weight(JUMP, mathfun_smooth_f32(motor.jumpPercent.current));
	override_weight(CROUCH, max_f32(crouchWeightFromJumping, crouchWeightFromLanding));
	
} // update_character()
