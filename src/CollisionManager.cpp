enum struct ColliderTag : int32
{
	Default,
	Trigger,
	Ladder,
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

	ColliderTag	tag;

	bool32 			hasCollision;
	Collision * 	collision;
};

struct CollisionManager
{
	ArenaArray<Handle<Collider>> colliders;

	// int32 collisionCount = 0;
	ArenaArray<Collision> collisions;

	Handle<Collider>
	PushCollider(	Handle<Transform3D> transform,
					Vector2 extents,
					Vector2 offset = {0, 0},
					ColliderTag tag = ColliderTag::Default)
	{
		auto index = colliders.Push(Handle<Collider>::create( 
		{
			.transform 	= transform,
			.extents 	= extents,
			.offset		= offset,
			.tag 		= tag
		}));
		return colliders[index];
	}

	bool32
	raycast(Vector2 origin, Vector2 ray, bool32 laddersBlock)
	{
		Vector2 corners [4];
		
		Vector2 start 	= origin;
		Vector2 end 	= origin + ray;

		for (int32 i = 0; i < colliders.count; ++i)
		{
			MAZEGAME_ASSERT(colliders[i]->transform.is_valid(), "Invalid transform passed to Collider");

			if (colliders[i]->tag == ColliderTag::Ladder && (laddersBlock == false))
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
	do_collisions() 
	{
		// Reset previous collisions
		flush_arena_array(&collisions);

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
				Vector2 positionA = Vector2{colliders[a]->transform->position.x, colliders[a]->transform->position.z} + colliders[a]->offset;
				Vector2 positionB = Vector2{colliders[b]->transform->position.x, colliders[b]->transform->position.z} + colliders[b]->offset;

				float xDeltaAtoB 	= positionB.x - positionA.x;
				float xDistance		= Abs(xDeltaAtoB);

				float xRadiusA = colliders[a]->extents.x;
				float xRadiusB = colliders[b]->extents.x;
				float xLimit = xRadiusA + xRadiusB;

				bool32 xCollision = xDistance < xLimit; 

				float zDeltaAtoB 	= positionB.y - positionA.y;
				float zDistance		= Abs(zDeltaAtoB);

				float zRadiusA = colliders[a]->extents.y;
				float zRadiusB = colliders[b]->extents.y;
				float zLimit = zRadiusA + zRadiusB;

				bool32 zCollision = zDistance < zLimit; 

				if (xCollision && zCollision)
				{
					if (colliders[a]->transform.get_index() == 0)
					{
						std::cout << "[COLLISIONS]: character collided with " << colliders[b]->transform.get_index() << ", at position " << positionA << "\n";
					}

					colliders[a]->hasCollision = true;
					colliders[b]->hasCollision = true;

					float xNormalA = Sign(xDeltaAtoB);
					float xNormalB = -xNormalA;

					uint64 collisionIndexForA = collisions.Push({
						.position 	= positionB.x + xNormalB * xRadiusB,
						.normal 	= {xNormalB, 0, 0},
						.tag 		= colliders[b]->tag
					});
					colliders[a]->collision = &collisions[collisionIndexForA];

					uint64 collisionIndexForB = collisions.Push({
						.position 	= positionA.x + xNormalA * xRadiusA, 
						.normal 	= {xNormalA, 0, 0},
						.tag 		= colliders[a]->tag
					});
					colliders[b]->collision = &collisions[collisionIndexForB];
				}
			}

		}
	}
};
