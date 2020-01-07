/*=============================================================================
Leo Tamminen 
shophorn @ intenet

3rd person character controller, nothhng less, nothing more
=============================================================================*/
struct CharacterController3rdPerson
{
	// References
	Handle<Transform3D> transform;

	// Properties
	float speed = 10;
	float collisionRadius = 0.25f;

	// State
	float 	zSpeed;
	Vector3 forward;

	Vector3 hitRayPosition;
	Vector3 hitRayNormal;
};

internal CharacterController3rdPerson
make_character(Handle<Transform3D> transform)
{
	CharacterController3rdPerson result =
	{
		.transform 	= transform,

		.zSpeed 	= 0.0f,
		.forward 	= get_forward(transform),
	};
	return result;
}

internal Vector3
ProcessCharacterInput(game::Input * input, Camera * camera)
{
	Vector3 viewForward = camera->forward;
	viewForward.z 		= 0;
	viewForward 		= vector::normalize(viewForward);
	Vector3 viewRight 	= vector::cross(viewForward, World::Up);

	Vector3 result = viewRight * input->move.x
					+ viewForward * input->move.y;

	return result;
}

void
update(	CharacterController3rdPerson * 	controller,
		game::Input * 					input,
		Camera * 						worldCamera,
		CollisionSystem3D *				collisionSystem,
		game::RenderInfo *				rendering)
{
	Vector3 movementVector = ProcessCharacterInput(input, worldCamera) * controller->speed * input->elapsedTime;

	Vector3 direction;
	float distance;
	vector::dissect(movementVector, &direction, &distance);

	if (distance > 0)
	{
		constexpr int rayCount = 5;
		Vector3 up 	= get_up(controller->transform);

		Vector3 rayStartPositions [rayCount];
		float sineStep 	= 2.0f / (rayCount - 1);

		rayStartPositions[0] = vector::rotate(direction * controller->collisionRadius, up, pi / 2.0f);
		for (int i = 1; i < rayCount; ++i)
		{
			float sine 		= -1.0f + (i - 1) * sineStep;
			float angle 	= -ArcCosine(sine);
			rayStartPositions[i] = vector::rotate(rayStartPositions[0], up, angle);
		}

	
		bool32 rayHit = false;
		RaycastResult raycastResult;
		for (int i  = 0; i < rayCount; ++i)
		{
			Vector3 start = rayStartPositions[i] + up * 0.25f + controller->transform->position;

			bool32 hit = raycast_3d(collisionSystem, start, direction, distance, &raycastResult);
			rayHit = rayHit || hit;

			rendering->draw_line(start, start + direction, {1, 1, 0});//hit ? Vector3{0, 1, 0} : Vector3 {1, 0, 0});

		}

		if (rayHit == false)
		{
			// controller->transform->position += movementVector;S
			controller->transform->position += direction * distance;
		}
		else
		{
			controller->hitRayPosition = controller->transform->position + up * 0.25f;
			controller->hitRayNormal = raycastResult.hitNormal;


			auto projectionOnNormal = vector::project(direction * distance, raycastResult.hitNormal);
			// controller->transform->position += direction * distance - projectionOnNormal;
		}

		rendering->draw_line(controller->hitRayPosition, vector::normalize(controller->hitRayNormal), {1,1,1});
		// rendering->draw_line(controller->hitRayPosition, controller->hitRayNormal, {1,1,1});



		float angleToWorldForward 		= vector::get_signed_angle(World::Forward, direction, World::Up);
		controller->transform->rotation = Quaternion::AxisAngle(World::Up, angleToWorldForward);
		
	}


	// const float epsilon = 0.001f;
	// if (Abs(input->move.x) > epsilon || Abs(input->move.y) > epsilon)
	// {
	// 	controller->forward 			= vector::normalize(movementVector);
	// 	Vector3 rayDirection;
	// 	float rayLength;
	// 	vector::dissect(movementVector, &rayDirection, &rayLength);

	// 	Vector3 rayStart = 	controller->transform->position
	// 						+ rayDirection * controller->collisionRadius
	// 						+ World::Up * 0.25f;

	// 	bool32 rayHit = raycast_3d(collisionSystem, rayStart, rayDirection, rayLength);

	// 	if(rayHit == false)
	// 		controller->transform->position = newPosition;


	// }	

	bool32 grounded = controller->transform->position.z < 0.1f;
	if (grounded && is_clicked(input->jump))
	{
		controller->zSpeed = 5;
	}

	controller->transform->position.z += controller->zSpeed * input->elapsedTime;

	if (controller->transform->position.z > 0)
	{	
		controller->zSpeed -= 2 * 9.81 * input->elapsedTime;
	}
	else
	{
		controller->zSpeed = 0;
        controller->transform->position.z = 0;
	}


}