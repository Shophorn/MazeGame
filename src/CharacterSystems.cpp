/*=============================================================================
Leo Tamminen
shophorn@protonmail.com

Player Character Systems
=============================================================================*/
#include <functional>

struct CharacterControllerSideScroller
{
	// References
	Transform3D * transform;
	Collider2D * collider;

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
			v2 leftRayOrigin 	= {	transform->position.x - collisionRadius, 
										transform->position.z + 0.5f};
			v2 leftRay 		= {xMovement, 0};
			bool32 leftRayHit 		= collisionManager->raycast(leftRayOrigin, leftRay, false);

			if (leftRayHit)
				xMovement = math::max(0.0f, xMovement);

			targetRotationRadians = pi / 2.0f;
		}

		// Going Right
		else if (xMovement > 0.0f)
		{
			v2 rightRayOrigin 	= {	transform->position.x + collisionRadius,
										transform->position.z + 0.5f};
			v2 rightRay 		= {xMovement, 0};
			bool32 rightRayHit		= collisionManager->raycast(rightRayOrigin, rightRay, false); 

			if (rightRayHit)
				xMovement = math::min(0.0f, xMovement);

			targetRotationRadians = -pi / 2.0f;
		}


		zSpeed += -9.81 * input->elapsedTime;

		if (collider->hasCollision && collider->collision->tag == ColliderTag::Ladder)
		{
			zSpeed = 0;			
		}

		float zMovement = 	moveStep * input->move.y
							+ zSpeed * input->elapsedTime;

		float skinWidth = 0.01f;
		v2 downRayOrigin 	= {	transform->position.x, 
									transform->position.z + skinWidth};
		v2 downRay 		= {0, zMovement - skinWidth};

		bool32 movingDown = input->move.y > -0.01f;
		bool32 downRayHit = collisionManager->raycast(downRayOrigin, downRay, movingDown);
		if (downRayHit)
		{
			zMovement = math::max(0.0f, zMovement);
			zSpeed = math::max(0.0f, zSpeed);
		}

		if ((math::absolute(zMovement) > math::absolute(xMovement)) && (math::absolute(xMovement / input->elapsedTime) < (0.1f * speed)))
		{
			targetRotationRadians = 0;
		}

		currentRotationRadians = interpolate(currentRotationRadians, targetRotationRadians, 0.4f);
		if (math::absolute(currentRotationRadians - targetRotationRadians) > (0.1 * pi))
		{
			xMovement = 0;
		}

		transform->position += {xMovement, 0, zMovement};
		transform->rotation = axis_angle_quaternion(up_v3, currentRotationRadians);

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