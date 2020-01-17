enum struct ColliderTag : s32
{
	Default,
	Trigger,
	Trigger2,
	Ladder,
};

struct Collision2D
{
	Vector3 		position;
	Vector3 		normal;
	ColliderTag 	tag;
};


struct Collider2D
{
	Handle<Transform3D>	transform;
	float2			extents;
	float2			offset;

	ColliderTag	tag;

	bool32 			hasCollision;
	Collision2D * 	collision;
};

struct CollisionManager2D
{
	ArenaArray<Handle<Collider2D>> colliders;
	ArenaArray<Collision2D> collisions;

	bool32
	raycast(float2 origin, float2 ray, bool32 laddersBlock)
	{
		float2 corners [4];
		
		float2 start 	= origin;
		float2 end 	= origin + ray;

		for (s32 i = 0; i < colliders.count(); ++i)
		{
			DEBUG_ASSERT(is_handle_valid(colliders[i]->transform), "Invalid transform passed to Collider2D");

			switch(colliders[i]->tag)
			{
				case ColliderTag::Default:
					// Note(Leo): This means we stay in loop, confusing much
					break;

				case ColliderTag::Trigger:
				case ColliderTag::Trigger2:
					continue;

				case ColliderTag::Ladder:
					if (laddersBlock == false)
						continue;
			}

			Vector3 worldPosition = get_world_position(colliders[i]->transform);
			float2 position2D = {	worldPosition.x, worldPosition.z };

			position2D += colliders[i]->offset;
			float2 extents = colliders[i]->extents;

			// Compute corners first
			corners[0] = position2D + float2 {-extents.x, -extents.y};
			corners[1] = position2D + float2 {extents.x, -extents.y};
			corners[2] = position2D + float2 {-extents.x, extents.y};
			corners[3] = position2D + float2 {extents.x, extents.y};

			// (0<AM⋅AB<AB⋅AB)∧(0<AM⋅AD<AD⋅AD)
			auto AMstart = start - corners[0];
			auto AMend = end - corners[0];
			auto AB = corners[1] - corners[0];
			auto AD = corners[2] - corners[0];

			bool32 startInside = 	(0 < vector::dot(AMstart, AB) && vector::dot(AMstart, AB) < vector::dot (AB, AB))
									&& (0 < vector::dot(AMstart, AD) && vector::dot(AMstart, AD) < vector::dot (AD, AD));

			bool32 endInside = 	(0 < vector::dot(AMend, AB) && vector::dot(AMend, AB) < vector::dot (AB, AB))
								&& (0 < vector::dot(AMend, AD) && vector::dot(AMend, AD) < vector::dot (AD, AD));

			if (!startInside && endInside)
				return true;
		}

		return false;
	}

	void
	do_collisions() 
	{
		// Reset previous collisions
		flush_arena_array(collisions);

		for (s32 i = 0; i < colliders.count(); ++i)
		{
			colliders[i]->hasCollision = false;
			colliders[i]->collision = nullptr;
		}

		// Calculate new collisions
		for (s32 a = 0; a < colliders.count(); ++a)
		{
			for (s32 b = a + 1; b < colliders.count(); ++b)
			{
				float2 positionA = float2{get_world_position(colliders[a]->transform).x, get_world_position(colliders[a]->transform).z} + colliders[a]->offset;
				float2 positionB = float2{get_world_position(colliders[b]->transform).x, get_world_position(colliders[b]->transform).z} + colliders[b]->offset;

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
					colliders[a]->hasCollision = true;
					colliders[b]->hasCollision = true;

					float xNormalA = Sign(xDeltaAtoB);
					float xNormalB = -xNormalA;

					u64 collisionIndexForA = push_one(collisions, {
						.position 	= positionB.x + xNormalB * xRadiusB,
						.normal 	= {xNormalB, 0, 0},
						.tag 		= colliders[b]->tag
					});
					colliders[a]->collision = &collisions[collisionIndexForA];

					u64 collisionIndexForB = push_one(collisions, {
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

internal Handle<Collider2D>
push_collider(	CollisionManager2D * manager,
				Handle<Transform3D> transform,
				float2 extents,
				float2 offset = {0, 0},
				ColliderTag tag = ColliderTag::Default)
{
	auto collider = make_handle<Collider2D>({
		.transform 	= transform,
		.extents 	= extents,
		.offset		= offset,
		.tag 		= tag
	});
	auto index = push_one(manager->colliders, collider);
	return collider;
}
