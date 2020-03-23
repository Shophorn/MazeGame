enum struct ColliderTag : s32
{
	Default,
	Trigger,
	Trigger2,
	Ladder,
};

struct Collision2D
{
	v3 		position;
	v3 		normal;
	ColliderTag 	tag;
};

struct Collider2D
{
	Transform3D *	transform;
	vector2			extents;
	vector2			offset;

	ColliderTag	tag;

	bool32 			hasCollision;
	Collision2D * 	collision;
};

struct CollisionManager2D
{
	Array<Collider2D> colliders;
	Array<Collision2D> collisions;

	bool32
	raycast(vector2 origin, vector2 ray, bool32 laddersBlock)
	{
		vector2 corners [4];
		
		vector2 start 	= origin;
		vector2 end 	= origin + ray;

		for (s32 i = 0; i < colliders.count(); ++i)
		{
			DEBUG_ASSERT(colliders[i].transform != nullptr, "Invalid transform passed to Collider2D");

			switch(colliders[i].tag)
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

			v3 worldPosition = get_world_position(*colliders[i].transform);
			vector2 position2D = {	worldPosition.x, worldPosition.z };

			position2D += colliders[i].offset;
			vector2 extents = colliders[i].extents;

			// Compute corners first
			corners[0] = position2D + vector2 {-extents.x, -extents.y};
			corners[1] = position2D + vector2 {extents.x, -extents.y};
			corners[2] = position2D + vector2 {-extents.x, extents.y};
			corners[3] = position2D + vector2 {extents.x, extents.y};

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
		// flush_arena_array(collisions);
		collisions.flush();

		for (s32 i = 0; i < colliders.count(); ++i)
		{
			colliders[i].hasCollision = false;
			colliders[i].collision = nullptr;
		}

		// Calculate new collisions
		for (s32 a = 0; a < colliders.count(); ++a)
		{
			for (s32 b = a + 1; b < colliders.count(); ++b)
			{
				vector2 positionA = vector2{get_world_position(*colliders[a].transform).x, get_world_position(*colliders[a].transform).z} + colliders[a].offset;
				vector2 positionB = vector2{get_world_position(*colliders[b].transform).x, get_world_position(*colliders[b].transform).z} + colliders[b].offset;

				float xDeltaAtoB 	= positionB.x - positionA.x;
				float xDistance		= Abs(xDeltaAtoB);

				float xRadiusA = colliders[a].extents.x;
				float xRadiusB = colliders[b].extents.x;
				float xLimit = xRadiusA + xRadiusB;

				bool32 xCollision = xDistance < xLimit; 

				float zDeltaAtoB 	= positionB.y - positionA.y;
				float zDistance		= Abs(zDeltaAtoB);

				float zRadiusA = colliders[a].extents.y;
				float zRadiusB = colliders[b].extents.y;
				float zLimit = zRadiusA + zRadiusB;

				bool32 zCollision = zDistance < zLimit; 

				if (xCollision && zCollision)
				{
					colliders[a].hasCollision = true;
					colliders[b].hasCollision = true;

					float xNormalA = Sign(xDeltaAtoB);
					float xNormalB = -xNormalA;

					u64 collisionIndexForA = collisions.count();
					collisions.push({
						.position 	= positionB.x + xNormalB * xRadiusB,
						.normal 	= {xNormalB, 0, 0},
						.tag 		= colliders[b].tag
					});
					colliders[a].collision = &collisions[collisionIndexForA];

					u64 collisionIndexForB = collisions.count();
					collisions.push({
						.position 	= positionA.x + xNormalA * xRadiusA, 
						.normal 	= {xNormalA, 0, 0},
						.tag 		= colliders[a].tag
					});
					colliders[b].collision = &collisions[collisionIndexForB];
				}
			}
		}
	}
};


internal Collider2D *
push_collider(	CollisionManager2D * 	manager,
				Transform3D * 			transform,
				vector2 				extents,
				vector2 				offset = {0, 0},
				ColliderTag 			tag = ColliderTag::Default)
{
	u64 index = manager->colliders.count();
	manager->colliders.push(
	{
		.transform 	= transform,
		.extents 	= extents,
		.offset		= offset,
		.tag 		= tag
	});
	return &manager->colliders[index];

	// auto collider = make_handle<Collider2D>({
	// 	.transform 	= transform,
	// 	.extents 	= extents,
	// 	.offset		= offset,
	// 	.tag 		= tag
	// });
	// push_one(manager->colliders, collider);
	// return collider;
}
