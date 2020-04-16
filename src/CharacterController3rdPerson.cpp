/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
template<s32 BufferSize>
struct BufferedValue
{	
	float buffer [BufferSize] 	= {};
	s32 position 				= 0;
	float value 				= 0;
};

template<s32 BufferSize>
void put_value(BufferedValue<BufferSize> & input, float value)
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
	float walkSpeed = 3;
	float runSpeed = 6;
	float jumpSpeed = 6;

	float gravity = -9.81;

	float collisionRadius = 0.25f;

	// State
	float 	zSpeed;
	v3 		forward;

	v3 hitRayPosition;
	v3 hitRayNormal;

	// Animations
	AnimatedSkeleton 	skeleton;

	Animation animations[CharacterAnimations::ANIMATION_COUNT];

	BufferedValue<10> fallPercent;
	BufferedValue<10> jumpPercent;
	BufferedValue<10> crouchPercent;
	BufferedValue<30> grounded;
};

internal CharacterController3rdPerson
make_character(Transform3D * transform)
{
	CharacterController3rdPerson result =
	{
		.transform 	= transform,

		.zSpeed 	= 0.0f,
		.forward 	= get_forward(*transform),
	};
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

	float inputMagnitude = inputVector.magnitude();

	float speed = -1;

	v4 gizmoColor = {0,0,0,1};

	constexpr float walkMinInput 	= 0.05f;
	constexpr float walkMaxInput 	= 0.5f;
	constexpr float runMinInput 	= 0.6f;
	constexpr float runMaxInput 	= 1.0001f; // account for epsilon

	constexpr float idleToWalkRange = walkMaxInput - walkMinInput; 
	constexpr float walkToRunRange 	= runMaxInput - runMinInput;

	using namespace CharacterAnimations;

	float weights [ANIMATION_COUNT] = {};

	auto override_weight = [&weights](AnimationType animation, float value)
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
	float crouchPercent = character.crouchPercent.value;

	constexpr float crouchOverridePowerForAnimation = 0.6f;
	constexpr float crouchOverridePowerForSpeed = 0.8f;

	if (inputMagnitude < walkMinInput)
	{
		weights[IDLE] 	= 1 * (1 - crouchPercent);
		weights[CROUCH] = 1 * crouchPercent;
		speed 			= 0;

		gizmoColor 		= {0, 0, 1, 1};
	}
	else if (inputMagnitude < walkMaxInput)
	{
		float t = (inputMagnitude - walkMinInput) / idleToWalkRange;

		weights[IDLE] 	= (1 - t) * (1 - crouchPercent);
		weights[CROUCH] = (1 - t) * crouchPercent;
		weights[WALK] 	= t;
		speed 			= interpolate(0, character.walkSpeed, t);

		float crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor 		= {1, 0, 0, 1};
	}
	else if (inputMagnitude < runMinInput)
	{
		weights[WALK] 	= 1;
		speed 			= character.walkSpeed;

		float crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor 		= {1, 1, 0, 1};
	}
	else
	{
		float t = (inputMagnitude - runMinInput) / walkToRunRange;

		weights[WALK] 	= 1 - t;
		weights[RUN] 	= t;
		speed 			= interpolate (character.walkSpeed, character.runSpeed, t);

		float crouchValue = character.crouchPercent.value;
		speed *= (1 - crouchValue * crouchOverridePowerForSpeed);
		override_weight(CROUCH, crouchValue * crouchOverridePowerForAnimation);

		gizmoColor = {0, 1, 0, 1};
	}

	// ------------------------------------------------------------------------

	put_value(character.fallPercent, character.zSpeed < - 1 ? (1 - character.grounded.value) : 0);
	override_weight(FALL, character.fallPercent.value);

	put_value(character.jumpPercent, character.zSpeed > 0.1 ? 1 : 0);
	override_weight(JUMP, character.jumpPercent.value);

	update_skeleton_animator(character.skeleton, animations, weights, ANIMATION_COUNT, input->elapsedTime);


	// ------------------------------------------------------------------------

	if (speed > 0)
	{
		v3 direction = inputVector / inputMagnitude;
		float distance = inputMagnitude * input->elapsedTime * speed;

		constexpr int rayCount = 5;
		v3 up 	= get_up(*character.transform);

		v3 rayStartPositions [rayCount];
		float sineStep 	= 2.0f / (rayCount - 1);

		rayStartPositions[0] = vector::rotate(direction * character.collisionRadius, up, pi / 2.0f);
		for (int i = 1; i < rayCount; ++i)
		{
			float sine 		= -1.0f + (i - 1) * sineStep;
			float angle 	= -arc_cosine(sine);
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

		float angleToWorldForward 		= vector::get_signed_angle(world::forward, direction, world::up);
		character.transform->rotation = quaternion::axis_angle(world::up, angleToWorldForward);
		
	}

	float groundHeight = get_terrain_height(collisionSystem, vector::convert_to<v2>(character.transform->position));
	bool32 grounded = character.transform->position.z < (0.1f + groundHeight);
	put_value(character.grounded, grounded ? 1 : 0);

	grounded = character.grounded.value > 0.5f;

	if (grounded && is_clicked(input->X))
	{
		character.zSpeed = character.jumpSpeed;
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
		logDebug(0) << "zSpeed: " << character.zSpeed;
	}

} // update_character()