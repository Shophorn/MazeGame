/*=============================================================================
Leo Tamminen
shophorn@protonmail.com

Player Character Systems
=============================================================================*/

struct Character
{
	Transform3D transform; 
	float zSpeed;
	float zRotationRadians;
};

enum struct ColliderTag : int32
{
	Default,
	Trigger,
};

struct Collision
{
	Vector3 		position;
	Vector3 		normal;
	ColliderTag 	tag;
};


struct Collider
{
	Transform3D * 	transform;
	Vector2			extents;
	Vector2			offset;

	bool32 			isLadder;
	ColliderTag	tag;

	bool32 			hasCollision;
	Collision * 	collision;
};

struct CollisionManager
{
	int32 colliderCount = 0;
	ArenaArray<Collider> colliders;

	int32 collisionCount = 0;
	ArenaArray<Collision> collisions;

	Collider *
	PushCollider(Transform3D * transform, Vector2 extents, Vector2 offset = {0, 0}, ColliderTag tag = ColliderTag::Default)
	{
		colliders[colliderCount] = 
		{
			.transform 	= transform,
			.extents 	= extents,
			.offset		= offset,
			.tag 		= tag
		};

		Collider * result = &colliders[colliderCount];
		colliderCount++;

		return result;
	}

	bool32
	Raycast(Vector2 origin, Vector2 ray, bool32 laddersBlock)
	{
		Vector2 corners [4];
		
		Vector2 start 	= origin;
		Vector2 end 	= origin + ray;

		for (int32 i = 0; i < colliderCount; ++i)
		{
			if (colliders[i].isLadder && (laddersBlock == false))
			{
				continue;
			}

			if (colliders[i].tag == ColliderTag::Trigger)
			{
				continue;
			}

			Vector3 worldPosition = colliders[i].transform->GetWorldPosition();
			Vector2 position2D = {	worldPosition.x, worldPosition.z };

			position2D += colliders[i].offset;
			Vector2 extents = colliders[i].extents;

			// Compute corners first
			corners[0] = position2D + Vector2 {-extents.x, -extents.y};
			corners[1] = position2D + Vector2 {extents.x, -extents.y};
			corners[2] = position2D + Vector2 {-extents.x, extents.y};
			corners[3] = position2D + Vector2 {extents.x, extents.y};

			// (0<AM⋅AB<AB⋅AB)∧(0<AM⋅AD<AD⋅AD)
			auto AMstart = start - corners[0];
			auto AMend = end - corners[0];
			auto AB = corners[1] - corners[0];
			auto AD = corners[2] - corners[0];

			bool32 startInside = 	(0 < Dot(AMstart, AB) && Dot(AMstart, AB) < Dot (AB, AB))
								&& (0 < Dot(AMstart, AD) && Dot(AMstart, AD) < Dot (AD, AD));

			bool32 endInside = 	(0 < Dot(AMend, AB) && Dot(AMend, AB) < Dot (AB, AB))
								&& (0 < Dot(AMend, AD) && Dot(AMend, AD) < Dot (AD, AD));

			if (!startInside && endInside)
				return true;
		}

		return false;
	}

	void
	DoCollisions() 
	{
		// Reset previous collisions
		collisionCount = 0;
		for (int32 i = 0; i < colliderCount; ++i)
		{
			colliders[i].hasCollision = false;
			colliders[i].collision = nullptr;
		}

		// Calculate new collisions
		for (int32 a = 0; a < colliderCount; ++a)
		{
			for (int32 b = a + 1; b < colliderCount; ++b)
			{
				float aPosition = colliders[a].transform->position.x;
				float bPosition = colliders[b].transform->position.x;
				float deltaAtoB = bPosition - aPosition;
				float distance 	= Abs(deltaAtoB);

				float aRadius = colliders[a].extents.x;
				float bRadius = colliders[b].extents.x;
				float limit = aRadius + bRadius;
				if (distance < limit)
				{
					colliders[a].hasCollision = true;
					colliders[b].hasCollision = true;

					// Collision for a
					int32 collisionIndexForA = collisionCount++;
					int32 collisionIndexForB = collisionCount++;

					float aNormalX = Sign(deltaAtoB);
					float bNormalX = -aNormalX;

					collisions[collisionIndexForA] =
					{
						.position 	= bPosition + bNormalX * bRadius, 
						.normal 	= {bNormalX, 0, 0},
						.tag 		= colliders[b].tag
					};
					colliders[a].collision = &collisions[collisionIndexForA];

					collisions[collisionIndexForB] =
					{
						.position 	= aPosition + aNormalX * aRadius, 
						.normal 	= {aNormalX, 0, 0},
						.tag 		= colliders[a].tag
					};
					colliders[b].collision = &collisions[collisionIndexForB];
				}
			}
		}
	}
};


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
	Character * character;
	Collider * collider;

	// Properties
	float speed 			= 10;
	float collisionRadius 	= 0.3f;
	float rotationSpeed		= 2 * pi;

	// State
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
			Vector2 leftRayOrigin 	= {	character->transform.position.x - collisionRadius, 
										character->transform.position.z + 0.5f};
			Vector2 leftRay 		= {xMovement, 0};
			bool32 leftRayHit 		= collisionManager->Raycast(leftRayOrigin, leftRay, false);

			if (leftRayHit)
				xMovement = Max(0.0f, xMovement);

			targetRotationRadians = -pi / 2.0f;
		}

		// Going Right
		else if (xMovement > 0.0f)
		{
			Vector2 rightRayOrigin 	= {	character->transform.position.x + collisionRadius,
										character->transform.position.z + 0.5f};
			Vector2 rightRay 		= {xMovement, 0};
			bool32 rightRayHit		= collisionManager->Raycast(rightRayOrigin, rightRay, false); 

			if (rightRayHit)
				xMovement = Min(0.0f, xMovement);

			targetRotationRadians = pi / 2.0f;
		}


		character->zSpeed += -9.81 * input->elapsedTime;

		float zMovement = 	moveStep * input->move.y
							+ character->zSpeed * input->elapsedTime;

		float skinWidth = 0.01f;
		Vector2 downRayOrigin 	= {	character->transform.position.x, 
									character->transform.position.z + skinWidth};
		Vector2 downRay 		= {0, zMovement - skinWidth};
		
		bool32 movingDown = input->move.y > -0.01f;

		bool32 downRayHit = collisionManager->Raycast(downRayOrigin, downRay, movingDown);
		if (downRayHit)
		{
			zMovement = Max(0.0f, zMovement);
			character->zSpeed = Max(0.0f, character->zSpeed);
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


		character->transform.position += {xMovement, 0, zMovement};

		character->transform.rotation = Quaternion::AxisAngle(World::Up, currentRotationRadians);



		if (collider->hasCollision && collider->collision->tag == ColliderTag::Trigger)
		{
			if (input->interact.IsClicked())
				testTriggered = !testTriggered;
		}

		character->transform.scale = testTriggered ? 2 : 1;
	}
};

struct CharacterController3rdPerson
{
	Character * character;

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
};