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
	Handle<Transform3D>	transform;
	Vector2			extents;
	Vector2			offset;

	bool32 			isLadder;
	ColliderTag	tag;

	bool32 			hasCollision;
	Collision * 	collision;
};

struct CollisionManager
{
	ArenaArray<Handle<Collider>> colliders;

	int32 collisionCount = 0;
	ArenaArray<Collision> collisions;

	Handle<Collider>
	PushCollider(Handle<Transform3D> transform, Vector2 extents, Vector2 offset = {0, 0}, ColliderTag tag = ColliderTag::Default)
	{
		auto index = colliders.Push(Handle<Collider>::Create( 
		{
			.transform 	= transform,
			.extents 	= extents,
			.offset		= offset,
			.tag 		= tag
		}));
		return colliders[index];
	}

	bool32
	Raycast(Vector2 origin, Vector2 ray, bool32 laddersBlock)
	{
		Vector2 corners [4];
		
		Vector2 start 	= origin;
		Vector2 end 	= origin + ray;

		for (int32 i = 0; i < colliders.count; ++i)
		{
			MAZEGAME_ASSERT(colliders[i]->transform.IsValid(), "Invalid transform passed to Collider");

			if (colliders[i]->isLadder && (laddersBlock == false))
			{
				continue;
			}

			if (colliders[i]->tag == ColliderTag::Trigger)
			{
				continue;
			}

			Vector3 worldPosition = colliders[i]->transform->GetWorldPosition();
			Vector2 position2D = {	worldPosition.x, worldPosition.z };

			position2D += colliders[i]->offset;
			Vector2 extents = colliders[i]->extents;

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
		for (int32 i = 0; i < colliders.count; ++i)
		{
			colliders[i]->hasCollision = false;
			colliders[i]->collision = nullptr;
		}

		// Calculate new collisions
		for (int32 a = 0; a < colliders.count; ++a)
		{
			for (int32 b = a + 1; b < colliders.count; ++b)
			{
				float aPosition = colliders[a]->transform->position.x;
				float bPosition = colliders[b]->transform->position.x;
				float deltaAtoB = bPosition - aPosition;
				float distance 	= Abs(deltaAtoB);

				float aRadius = colliders[a]->extents.x;
				float bRadius = colliders[b]->extents.x;
				float limit = aRadius + bRadius;
				if (distance < limit)
				{
					colliders[a]->hasCollision = true;
					colliders[b]->hasCollision = true;

					// Collision for a
					int32 collisionIndexForA = collisionCount++;
					int32 collisionIndexForB = collisionCount++;

					float aNormalX = Sign(deltaAtoB);
					float bNormalX = -aNormalX;

					collisions[collisionIndexForA] =
					{
						.position 	= bPosition + bNormalX * bRadius, 
						.normal 	= {bNormalX, 0, 0},
						.tag 		= colliders[b]->tag
					};
					colliders[a]->collision = &collisions[collisionIndexForA];

					collisions[collisionIndexForB] =
					{
						.position 	= aPosition + aNormalX * aRadius, 
						.normal 	= {aNormalX, 0, 0},
						.tag 		= colliders[a]->tag
					};
					colliders[b]->collision = &collisions[collisionIndexForB];
				}
			}
		}
	}
};
