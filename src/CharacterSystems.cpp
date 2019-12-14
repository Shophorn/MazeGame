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
	float 			radius;

	bool 			hasCollision;
	Collision * 	collision;
};

struct CollisionManager
{
	int runningColliderIndex = 0;
	ArenaArray<Collider> colliders;

	int runningCollisionIndex = 0;
	ArenaArray<Collision> collisions;

	Collider *
	PushCollider(Transform3D * transform, float radius)
	{
		colliders[runningColliderIndex] = 
		{
			.transform 	= transform,
			.radius 	= radius,
		};

		Collider * result = &colliders[runningColliderIndex];
		runningColliderIndex++;

		return result;
	}

	void
	DoCollisions()
	{

		// Reset previous collisions
		runningCollisionIndex = 0;
		for (int i = 0; i < runningColliderIndex; ++i)
		{
			colliders[i].hasCollision = false;
			colliders[i].collision = nullptr;
		}

		// Calculate new collisions
		for (int a = 0; a < runningColliderIndex; ++a)
		{
			for (int b = a + 1; b < runningColliderIndex; ++b)
			{
				float aPosition = colliders[a].transform->position.x;
				float bPosition = colliders[b].transform->position.x;
				float deltaAtoB = bPosition - aPosition;
				float distance 	= Abs(deltaAtoB);

				float aRadius = colliders[a].radius;
				float bRadius = colliders[b].radius;
				float limit = aRadius + bRadius;
				if (distance < limit)
				{
					colliders[a].hasCollision = true;
					colliders[b].hasCollision = true;

					// Collision for a
					int collisionIndexForA = runningCollisionIndex++;
					int collisionIndexForB = runningCollisionIndex++;

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
	Update(game::Input * input, ArenaArray<Rectangle> * colliders)
	{
		if (collider->hasCollision)
		{
			character->transform.scale = 2;
		}
		else
		{
			character->transform.scale = 1;
		}

		float moveStep = speed * input->elapsedTime;
		float xMovement = moveStep * input->move.x;	
		if (collider->hasCollision)
		{	
			float dot = Dot(collider->collision->normal, {xMovement, 0, 0});
			std::cout << collider->collision->normal << ", " << Vector3 {xMovement, 0, 0} << ", " << dot << "\n";
			if (Sign(dot) < 0)
			{
				xMovement = 0;
			}
		}

		float zMovement = moveStep * input->move.y;

		character->transform.position += {xMovement, 0, zMovement};
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