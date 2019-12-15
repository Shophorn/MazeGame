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

struct Collision
{
	Vector3 position;
	Vector3 normal;
};

struct Collider
{
	Transform3D * 	transform;
	Vector2			extents;
	Vector2			offset;

	bool 			hasCollision;
	Collision * 	collision;
};

struct CollisionManager
{
	int colliderCount = 0;
	ArenaArray<Collider> colliders;

	int collisionCount = 0;
	ArenaArray<Collision> collisions;

	Collider *
	PushCollider(Transform3D * transform, Vector2 extents, Vector2 offset = {0, 0})
	{
		colliders[colliderCount] = 
		{
			.transform 	= transform,
			.extents 	= extents,
			.offset		= offset
		};

		Collider * result = &colliders[colliderCount];
		colliderCount++;

		return result;
	}

	bool32
	Raycast(Vector2 origin, Vector2 ray)
	{
		Vector2 corners [4];
		
		Vector2 start 	= origin;
		Vector2 end 	= origin + ray;

		for (int i = 0; i < colliderCount; ++i)
		{
			Vector2 position2D = {	colliders[i].transform->position.x,
									colliders[i].transform->position.z};

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

			bool startInside = 	(0 < Dot(AMstart, AB) && Dot(AMstart, AB) < Dot (AB, AB))
								&& (0 < Dot(AMstart, AD) && Dot(AMstart, AD) < Dot (AD, AD));

			bool endInside = 	(0 < Dot(AMend, AB) && Dot(AMend, AB) < Dot (AB, AB))
								&& (0 < Dot(AMend, AD) && Dot(AMend, AD) < Dot (AD, AD));

			// std::cout << "\t" << position2D << ", " << extents << ", " << (startInside ? "True" : "False") << ", " << (endInside ? "True" : "False") << "\n";

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
		for (int i = 0; i < colliderCount; ++i)
		{
			colliders[i].hasCollision = false;
			colliders[i].collision = nullptr;
		}

		// Calculate new collisions
		for (int a = 0; a < colliderCount; ++a)
		{
			for (int b = a + 1; b < colliderCount; ++b)
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
					int collisionIndexForA = collisionCount++;
					int collisionIndexForB = collisionCount++;

					float aNormalX = Sign(deltaAtoB);
					float bNormalX = -aNormalX;

					collisions[collisionIndexForA] =
					{
						.position = bPosition + bNormalX * bRadius, 
						.normal = {bNormalX, 0, 0},
					};
					colliders[a].collision = &collisions[collisionIndexForA];

					collisions[collisionIndexForB] =
					{
						.position = aPosition + aNormalX * aRadius, 
						.normal = {aNormalX, 0, 0},
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
	Character * character;
	Collider * collider;

	float speed 			= 10;
	float collisionRadius 	= 0.5f;

	void
	Update(game::Input * input, CollisionManager * collisionManager)
	{
		float moveStep = speed * input->elapsedTime;

		float xMovement = moveStep * input->move.x;

		Vector2 leftRayOrigin 	= {	character->transform.position.x - collisionRadius, 
									character->transform.position.z + 0.5f};
		Vector2 leftRay 		= {xMovement, 0};
		bool32 leftRayHit 		= collisionManager->Raycast(leftRayOrigin, leftRay);

		Vector2 rightRayOrigin 	= {	character->transform.position.x + collisionRadius,
									character->transform.position.z + 0.5f};
		Vector2 rightRay 		= {xMovement, 0};
		bool32 rightRayHit		= collisionManager->Raycast(rightRayOrigin, rightRay); 


		// Todo(Leo): Do not just 0 movement, but onyl shorten it the required amount
		if (leftRayHit)
			xMovement = Max(0.0f, xMovement);

		if (rightRayHit)
			xMovement = Min(0.0f, xMovement);




		character->zSpeed += -9.81 * input->elapsedTime;

		float zMovement = 	moveStep * input->move.y
							+ character->zSpeed * input->elapsedTime;

		float skinWidth = 0.01f;
		Vector2 downRayOrigin 	= {	character->transform.position.x, 
									character->transform.position.z + skinWidth};
		Vector2 downRay 		= {0, zMovement - skinWidth};
		
		bool32 downRayHit = collisionManager->Raycast(downRayOrigin, downRay);
		if (downRayHit)
		{
			zMovement = Max(0.0f, zMovement);
			character->zSpeed = 0;

			std::cout << "Down ray hit\n";
		}
		else
		{
			std::cout << "Down ray NOT hit\n";
		}

		character->transform.position += {xMovement, 0, zMovement};
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