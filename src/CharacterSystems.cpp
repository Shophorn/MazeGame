/*=============================================================================
Leo Tamminen
shophorn@protonmail.com

Player Character Systems
=============================================================================*/

internal Vector3
ProcessCharacterInput(game::Input * input, Camera * camera)
{
	Vector3 viewForward = camera->forward;
	viewForward.z 		= 0;
	viewForward 		= Normalize(viewForward);
	Vector3 viewRight 	= Cross(viewForward, World::Up);

	Vector3 result = viewRight * input->move.x
					+ viewForward * input->move.y;

	return result;
}

struct CharacterControllerSideScroller
{
	// References
	Handle<Transform3D> transform;
	Collider * collider;

	// Properties
	float speed 			= 10;
	float collisionRadius 	= 0.3f;
	float rotationSpeed		= 2 * pi;

	// State
	float zSpeed;
	float zRotationRadians;
	float targetRotationRadians = 0;
	float currentRotationRadians = 0;
	bool32 testTriggered = false;

	void
	Update(game::Input * input, CollisionManager * collisionManager)
	{
		float moveStep = speed * input->elapsedTime;
		float xMovement = moveStep * input->move.x;

		// Going Left
		if (xMovement < 0.0f)
		{
			Vector2 leftRayOrigin 	= {	transform->position.x - collisionRadius, 
										transform->position.z + 0.5f};
			Vector2 leftRay 		= {xMovement, 0};
			bool32 leftRayHit 		= collisionManager->Raycast(leftRayOrigin, leftRay, false);

			if (leftRayHit)
				xMovement = Max(0.0f, xMovement);

			targetRotationRadians = -pi / 2.0f;
		}

		// Going Right
		else if (xMovement > 0.0f)
		{
			Vector2 rightRayOrigin 	= {	transform->position.x + collisionRadius,
										transform->position.z + 0.5f};
			Vector2 rightRay 		= {xMovement, 0};
			bool32 rightRayHit		= collisionManager->Raycast(rightRayOrigin, rightRay, false); 

			if (rightRayHit)
				xMovement = Min(0.0f, xMovement);

			targetRotationRadians = pi / 2.0f;
		}

		zSpeed += -9.81 * input->elapsedTime;

		float zMovement = 	moveStep * input->move.y
							+ zSpeed * input->elapsedTime;

		float skinWidth = 0.01f;
		Vector2 downRayOrigin 	= {	transform->position.x, 
									transform->position.z + skinWidth};
		Vector2 downRay 		= {0, zMovement - skinWidth};

		bool32 movingDown = input->move.y > -0.01f;
		bool32 downRayHit = collisionManager->Raycast(downRayOrigin, downRay, movingDown);
		if (downRayHit)
		{
			zMovement = Max(0.0f, zMovement);
			zSpeed = Max(0.0f, zSpeed);
		}

		if ((Abs(zMovement) > Abs(xMovement)) && (Abs(xMovement / input->elapsedTime) < (0.1f * speed)))
		{
			targetRotationRadians = 0;
		}

		currentRotationRadians = Interpolate(currentRotationRadians, targetRotationRadians, 0.4f, 2);
		if (Abs(currentRotationRadians - targetRotationRadians) > (0.1 * pi))
		{
			xMovement = 0;
		}

		transform->position += {xMovement, 0, zMovement};

		transform->rotation = Quaternion::AxisAngle(World::Up, currentRotationRadians);


		if (collider->hasCollision && collider->collision->tag == ColliderTag::Trigger)
		{
			if (input->interact.IsClicked())
				testTriggered = !testTriggered;
		}

		transform->scale = testTriggered ? 2 : 1;
	}
};

struct CharacterController3rdPerson
{
	/*
	Transform3D * transform;


	float speed = 10;
	float collisionRadius = 0.5f;

	void
	Update(	game::Input * input,
			Camera * worldCamera,
			ArenaArray<Rectangle> * colliders)
	{
		bool32 grounded = character->transform.position.z < 0.1f;

		Vector3 characterMovementVector = ProcessCharacterInput(input, worldCamera);

		Vector3 characterNewPosition = character->transform.position + characterMovementVector * speed * input->elapsedTime;

		// Collisions
		Circle characterCollisionCircle = {characterNewPosition.x, characterNewPosition.y, collisionRadius};

		CollisionResult collisionResult = GetCollisions(characterCollisionCircle, colliders);

		if (collisionResult.isCollision == false)
		{
			character->transform.position = characterNewPosition;
		}

		if (grounded && input->jump.IsClicked())
		{
			character->zSpeed = 5;
		}

		character->transform.position.z += character->zSpeed * input->elapsedTime;

		if (character->transform.position.z > 0)
		{	
			character->zSpeed -= 2 * 9.81 * input->elapsedTime;
		}
		else
		{
			character->zSpeed = 0;
	        character->transform.position.z = 0;
		}

		
		float epsilon = 0.001f;
		if (Abs(input->move.x) > epsilon || Abs(input->move.y) > epsilon)
		{
			Vector3 characterForward = Normalize(characterMovementVector);
			float angleToWorldForward = SignedAngle(World::Forward, characterForward, World::Up);
			character->zRotationRadians = angleToWorldForward;
		}

		character->transform.rotation = Quaternion::AxisAngle(World::Up, character->zRotationRadians);
	}
	*/
};