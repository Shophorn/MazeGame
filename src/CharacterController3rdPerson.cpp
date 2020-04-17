/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
template<s32 BufferSize>
struct BufferedValue
{	
	f32 buffer [BufferSize] 	= {};
	s32 position 				= 0;
	f32 value 				= 0;
};

template<s32 BufferSize>
void put_value(BufferedValue<BufferSize> & input, f32 value)
{
	input.value -= input.buffer[input.position];
	input.buffer[input.position] = value / BufferSize;
	input.value += input.buffer[input.position];

	input.position += 1;
	input.position %= BufferSize;
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

	// These numbers are frames to average in buffer. It is bad since it does not consider frame rate at all
	// Todo(Leo): Fix
	BufferedValue<15> fallPercent;
	BufferedValue<10> jumpPercent;
	BufferedValue<10> crouchPercent;
	BufferedValue<30> grounded;

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

	f32 currentSpeed;
	/* Note(Leo): As in change of velocity per time unit. These probably
	should be kept at values relative to max speed */ 
	f32 acceleration = 2.0f * runSpeed;
	f32 deceleration = -4.0f * runSpeed;
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
	v3 viewForward = get_forward(camera);
	viewForward.z 		= 0;
	viewForward 		= viewForward.normalized();
	v3 viewRight 	= vector::cross(viewForward, world::up);

	v3 result = viewRight * input->move.x
					+ viewForward * input->move.y;

	return result;
}

void
update_character(	
		CharacterController3rdPerson & 	character,
		game::Input * 					input,
		Camera * 						worldCamera,
		CollisionSystem3D *				collisionSystem,
		platform::Graphics * 	graphics,
		platform::Functions * 	functions)
{
	v3 inputVector = process_player_input(input, worldCamera);

	f32 inputMagnitude = inputVector.magnitude();


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
		for (int i = 0; i < ANIMATION_COUNT; ++i)
		{
			weights[i] *= 1 - value;

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

	put_value(character.crouchPercent, is_pressed(input->B) ? 1 : 0);
	f32 crouchPercent = character.crouchPercent.value;

	constexpr f32 crouchOverridePowerForAnimation = 0.6f;
	constexpr f32 crouchOverridePowerForSpeed = 0.8f;

	f32 speed = 0;

	if (inputMagnitude < walkMinInput)
	{
		weights[IDLE] 	= 1 * (1 - crouchPercent);
		weights[CROUCH] = 1 * crouchPercent;
		speed 			= 0;

		gizmoColor 		= {0, 0, 1, 1};
	}
	else if (inputMagnitude < walkMaxInput)
	{
		f32 t = (inputMagnitude - walkMinInput) / idleToWalkRange;

		weights[IDLE] 	= (1 - t) * (1 - crouchPercent);
		weights[CROUCH] = (1 - t) * crouchPercent;
		weights[WALK] 	= t;
		speed 			= interpolate(0, character.walkSpeed, t);

		f32 crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor 		= {1, 0, 0, 1};
	}
	else if (inputMagnitude < runMinInput)
	{
		weights[WALK] 	= 1;
		speed 			= character.walkSpeed;

		f32 crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor 		= {1, 1, 0, 1};
	}
	else
	{
		f32 t = (inputMagnitude - runMinInput) / walkToRunRange;

		weights[WALK] 	= 1 - t;
		weights[RUN] 	= t;
		speed 			= interpolate (character.walkSpeed, character.runSpeed, t);

		f32 crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor = {0, 1, 0, 1};
	}

	// ------------------------------------------------------------------------

	put_value(character.fallPercent, character.zSpeed < - 1 ? (1 - character.grounded.value) : 0);
	override_weight(FALL, math::smooth(character.fallPercent.value));

	put_value(character.jumpPercent, character.zSpeed > 0.1 ? 1 : 0);
	override_weight(JUMP, math::smooth(character.jumpPercent.value));

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

	speed *= 1 - landingValue;

	// f32 currentSpeed;

	// if (speed > character.currentSpeed)
	// {
	// 	currentSpeed = character.currentSpeed + character.acceleration * input->elapsedTime;
	// 	currentSpeed = math::min(currentSpeed, speed);

	// 	// character.currentSpeed += character.acceleration * input->elapsedTime;
	// 	// character.currentSpeed = math::min(character.currentSpeed, speed);
	// }
	// else if (speed < character.currentSpeed)
	// {
	// 	currentSpeed = character.currentSpeed + character.deceleration * input->elapsedTime;
	// 	currentSpeed = math::max(currentSpeed, speed);

	// 	currentSpeed = speed;
	// 	// character.currentSpeed = speed;

	// 	// float a = character.currentSpeed + character.deceleration * input->elapsedTime;
	// 	// a = math::max(a, speed);
	// 	// logDebug(0) << "a: " << a;


	// 	// character.currentSpeed += character.deceleration * input->elapsedTime;
	// 	// character.currentSpeed = math::max(character.currentSpeed, speed);
	// }

	// // if (speed != speed)
	// // {
	// // 	logDebug(0) << "speed is nan!!";
	// // 	speed = 0;
	// // }

	// // if (character.currentSpeed != character.currentSpeed)
	// // {
	// // 	logDebug(0) << "currentSpeed is nan!!";
	// // }

	// // logDebug(0) << "speeds: " << character.currentSpeed << "/" << speed;

	// // speed = character.currentSpeed;
	// currentSpeed = speed;

	// character.currentSpeed = currentSpeed;

	if (speed > 0)
	{
		v3 direction = inputVector / inputMagnitude;
		f32 distance = inputMagnitude * input->elapsedTime * speed;

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
			// functions->draw_line(graphics, start, start + direction, 5.0f, lineColor);

		}

		if (rayHit == false)
		{
			character.transform->position += direction * distance;
		}
		else
		{
			character.hitRayPosition = character.transform->position + up * 0.25f;
			character.hitRayNormal = raycastResult.hitNormal;

			auto projectionOnNormal = vector::project(direction * distance, raycastResult.hitNormal);
			// character.transform->position += direction * distance - projectionOnNormal;
		}

		f32 angleToWorldForward 		= vector::get_signed_angle(world::forward, direction, world::up);
		character.transform->rotation = quaternion::axis_angle(world::up, angleToWorldForward);		
	}

	f32 groundHeight = get_terrain_height(collisionSystem, vector::convert_to<v2>(character.transform->position));
	bool32 grounded = character.transform->position.z < (0.1f + groundHeight);

	bool32 startLanding = character.wasGroundedLastFrame == false && grounded == true;
	character.wasGroundedLastFrame = grounded;


	put_value(character.grounded, grounded ? 1 : 0);

	grounded = character.grounded.value > 0.5f;

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
			character.zSpeed = character.jumpSpeed * 5;
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
	m44 gizmoTransform = make_transform_matrix(	character.transform->position + v3{0,0,2}, 0.3f);
	debug::draw_diamond(graphics, gizmoTransform, gizmoColor, functions);

	if (grounded)
	{
		gizmoTransform = make_transform_matrix(character.transform->position + v3{0,0,2}, 0.1);
		debug::draw_diamond(graphics, gizmoTransform, gizmoColor, functions);
	}
	else
	{
		// logDebug(0) << "zSpeed: " << character.zSpeed;
	}

} // update_character()