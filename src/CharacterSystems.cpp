/*=============================================================================
Leo Tamminen
shophorn@protonmail.com

Player Character Systems
=============================================================================*/
#include <functional>


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
	Handle<Collider2D> collider;

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

	using LadderTriggerFunc = std::function<void()>;

	LadderTriggerFunc * OnTriggerLadder1;
	LadderTriggerFunc * OnTriggerLadder2;

	void
	update(game::Input * input, CollisionManager2D * collisionManager)
	{
		float moveStep = speed * input->elapsedTime;
		float xMovement = moveStep * input->move.x;

		// Going Left
		if (xMovement < 0.0f)
		{
			Vector2 leftRayOrigin 	= {	transform->position.x - collisionRadius, 
										transform->position.z + 0.5f};
			Vector2 leftRay 		= {xMovement, 0};
			bool32 leftRayHit 		= collisionManager->raycast(leftRayOrigin, leftRay, false);

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
			bool32 rightRayHit		= collisionManager->raycast(rightRayOrigin, rightRay, false); 

			if (rightRayHit)
				xMovement = Min(0.0f, xMovement);

			targetRotationRadians = pi / 2.0f;
		}


		zSpeed += -9.81 * input->elapsedTime;

		if (collider->hasCollision && collider->collision->tag == ColliderTag::Ladder)
		{
			zSpeed = 0;			
		}

		float zMovement = 	moveStep * input->move.y
							+ zSpeed * input->elapsedTime;

		float skinWidth = 0.01f;
		Vector2 downRayOrigin 	= {	transform->position.x, 
									transform->position.z + skinWidth};
		Vector2 downRay 		= {0, zMovement - skinWidth};

		bool32 movingDown = input->move.y > -0.01f;
		bool32 downRayHit = collisionManager->raycast(downRayOrigin, downRay, movingDown);
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
			if (is_clicked(input->interact))
			{
				(*OnTriggerLadder1)();
			}
		}

		if (collider->hasCollision && collider->collision->tag == ColliderTag::Trigger2)
		{
			if (is_clicked(input->interact))
			{
				(*OnTriggerLadder2)();
			}
		}

	}
};

struct CharacterController3rdPerson
{
	// References
	Handle<Transform3D> transform;

	// Properties
	float speed = 10;
	float collisionRadius = 0.5f;

	// State
	float 	zSpeed;
	Vector3 forward;
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

void
update(	CharacterController3rdPerson * 	controller,
		game::Input * 					input,
		Camera * 						worldCamera,
		CollisionSystem3D *				collisionSystem)
{
	bool32 grounded = controller->transform->position.z < 0.1f;

	Vector3 movementVector 	= ProcessCharacterInput(input, worldCamera) * controller->speed * input->elapsedTime;
	Vector3 newPosition 	= controller->transform->position + movementVector;

	const float epsilon = 0.001f;
	if (Abs(input->move.x) > epsilon || Abs(input->move.y) > epsilon)
	{
		controller->forward = Normalize(movementVector);
		float angleToWorldForward = SignedAngle(World::Forward, controller->forward, World::Up);
		controller->transform->rotation = Quaternion::AxisAngle(World::Up, angleToWorldForward);
		

		Vector3 rayDirection;
		float rayLength;
		vector::dissect(movementVector, &rayDirection, &rayLength);

		Vector3 rayStart = 	controller->transform->position
							+ rayDirection * controller->collisionRadius
							+ World::Up * 0.25f;
		bool32 rayHit = raycast_3d(collisionSystem, rayStart, rayDirection, rayLength);

		if(rayHit == false)
			controller->transform->position = newPosition;


	}	


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