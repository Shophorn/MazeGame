/*=============================================================================
Leo Tamminen 
shophorn @ internet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
struct CharacterController3rdPerson
{
	// References
	Transform3D * transform;

	// Properties
	float speed = 6;
	float collisionRadius = 0.25f;

	// State
	float 	zSpeed;
	v3 		forward;

	v3 hitRayPosition;
	v3 hitRayNormal;

	// Animations
	SkeletonAnimator 	animator;
	Animation 			idleAnimation;
	Animation 			walkAnimation;
	Animation 			runAnimation;
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
	v3 movementVector = inputVector * character.speed * input->elapsedTime;

	v3 direction;
	float distance;
	vector::dissect(movementVector, &direction, &distance);

	if (distance > 0)
	{
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
			functions->draw_line(graphics, start, start + direction, 5.0f, lineColor);

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
	debug::draw_axes(	graphics,
						get_matrix(*character.transform),
						0.5f,
						functions);

	float groundHeight = get_terrain_height(collisionSystem, {character.transform->position.x, character.transform->position.y});
	bool32 grounded = character.transform->position.z < 0.1f + groundHeight;
	if (grounded && is_clicked(input->jump))
	{
		character.zSpeed = 5;
	}

	character.transform->position.z += character.zSpeed * input->elapsedTime;

	if (character.transform->position.z > groundHeight)
	{	
		character.zSpeed -= 2 * 9.81 * input->elapsedTime;
	}
	else
	{
		character.zSpeed = 0;
        character.transform->position.z = groundHeight;
	}


	s32 moveStage = math::ceil_to_s32(inputVector.magnitude() * 2);
	switch(moveStage)
	{
		case 0: character.animator.animation = &character.idleAnimation; break;
		case 1: character.animator.animation = &character.walkAnimation; break;
		case 2: character.animator.animation = &character.runAnimation; break;

		default:
			logConsole(1) 	<< FILE_ADDRESS 
							<< "Bad input value(" << inputVector.magnitude() 
							<< ") resulted in bad move stage (" << moveStage << ")";
			break;
	}

} // update_character()