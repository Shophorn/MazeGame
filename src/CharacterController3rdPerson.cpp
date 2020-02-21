/*=============================================================================
Leo Tamminen 
shophorn @ intenet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
struct CharacterController3rdPerson
{
	// References
	Transform3D * transform;

	// Properties
	float speed = 10;
	float collisionRadius = 0.25f;

	// State
	float 	zSpeed;
	v3 forward;

	v3 hitRayPosition;
	v3 hitRayNormal;
};

internal CharacterController3rdPerson
make_character(Transform3D * transform)
{
	CharacterController3rdPerson result =
	{
		.transform 	= transform,

		.zSpeed 	= 0.0f,
		.forward 	= get_forward(transform),
	};
	return result;
}

internal v3
process_player_input(game::Input * input, Camera * camera)
{
	v3 viewForward = get_forward(camera);
	viewForward.z 		= 0;
	viewForward 		= vector::normalize(viewForward);
	v3 viewRight 	= vector::cross(viewForward, world::up);

	v3 result = viewRight * input->move.x
					+ viewForward * input->move.y;

	return result;
}

void
update_character(	
		CharacterController3rdPerson * 	controller,
		game::Input * 					input,
		Camera * 						worldCamera,
		CollisionSystem3D *				collisionSystem,
		platform::Graphics * 	graphics,
		platform::Functions * 	functions)
{
	v3 movementVector = process_player_input(input, worldCamera) * controller->speed * input->elapsedTime;

	v3 direction;
	float distance;
	vector::dissect(movementVector, &direction, &distance);

	if (distance > 0)
	{
		constexpr int rayCount = 5;
		v3 up 	= get_up(controller->transform);

		v3 rayStartPositions [rayCount];
		float sineStep 	= 2.0f / (rayCount - 1);

		rayStartPositions[0] = vector::rotate(direction * controller->collisionRadius, up, pi / 2.0f);
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
			v3 start = rayStartPositions[i] + up * 0.25f + controller->transform->position;

			bool32 hit = raycast_3d(collisionSystem, start, direction, distance, &raycastResult);
			rayHit = rayHit || hit;

			float4 lineColor = hit ? float4{0,1,0,1} : float4{1,0,0,1};
			functions->draw_line(graphics, start, start + direction, 5.0f, lineColor);

		}

		if (rayHit == false)
		{
			controller->transform->position += direction * distance;
		}
		else
		{
			controller->hitRayPosition = controller->transform->position + up * 0.25f;
			controller->hitRayNormal = raycastResult.hitNormal;


			auto projectionOnNormal = vector::project(direction * distance, raycastResult.hitNormal);
			// controller->transform->position += direction * distance - projectionOnNormal;
		}

		float angleToWorldForward 		= vector::get_signed_angle(world::forward, direction, world::up);
		controller->transform->rotation = quaternion::axis_angle(world::up, angleToWorldForward);
		
	}

	float groundHeight = get_terrain_height(collisionSystem, {controller->transform->position.x, controller->transform->position.y});
	bool32 grounded = controller->transform->position.z < 0.1f + groundHeight;
	if (grounded && is_clicked(input->jump))
	{
		controller->zSpeed = 5;
	}

	controller->transform->position.z += controller->zSpeed * input->elapsedTime;

	if (controller->transform->position.z > groundHeight)
	{	
		controller->zSpeed -= 2 * 9.81 * input->elapsedTime;
	}
	else
	{
		controller->zSpeed = 0;
        controller->transform->position.z = groundHeight;
	}


}