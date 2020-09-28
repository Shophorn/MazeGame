// enum struct ColliderTag : s32
// {
// 	Default,
// 	Trigger,
// 	Trigger2,
// 	Ladder,
// };

// struct Collision2D
// {
// 	v3 		position;
// 	v3 		normal;
// 	ColliderTag 	tag;
// };

// struct Collider2D
// {
// 	Transform3D *	transform;
// 	v2			extents;
// 	v2			offset;

// 	ColliderTag	tag;

// 	bool32 			hasCollision;
// 	Collision2D * 	collision;
// };

// struct CollisionManager2D
// {
// 	Array2<Collider2D> colliders;
// 	Array2<Collision2D> collisions;

// 	bool32
// 	raycast(v2 origin, v2 ray, bool32 laddersBlock)
// 	{
// 		v2 corners [4];
		
// 		v2 start 	= origin;
// 		v2 end 	= origin + ray;

// 		for (s32 i = 0; i < colliders.count(); ++i)
// 		{
// 			AssertMsg(colliders[i].transform != nullptr, "Invalid transform passed to Collider2D");

// 			switch(colliders[i].tag)
// 			{
// 				case ColliderTag::Default:
// 					// Note(Leo): This means we stay in loop, confusing much
// 					break;

// 				case ColliderTag::Trigger:
// 				case ColliderTag::Trigger2:
// 					continue;

// 				case ColliderTag::Ladder:
// 					if (laddersBlock == false)
// 						continue;
// 			}

// 			// v3 worldPosition = get_world_position(*colliders[i].transform);
// 			v3 worldPosition = colliders[i].transform->position;
// 			v2 position2D = {worldPosition.x, worldPosition.z };

// 			position2D += colliders[i].offset;
// 			v2 extents = colliders[i].extents;

// 			// Compute corners first
// 			corners[0] = position2D + v2 {-extents.x, -extents.y};
// 			corners[1] = position2D + v2 {extents.x, -extents.y};
// 			corners[2] = position2D + v2 {-extents.x, extents.y};
// 			corners[3] = position2D + v2 {extents.x, extents.y};

// 			// (0<AM⋅AB<AB⋅AB)∧(0<AM⋅AD<AD⋅AD)
// 			auto AMstart = start - corners[0];
// 			auto AMend = end - corners[0];
// 			auto AB = corners[1] - corners[0];
// 			auto AD = corners[2] - corners[0];

// 			bool32 startInside = 	(0 < dot_v2(AMstart, AB) && dot_v2(AMstart, AB) < dot_v2 (AB, AB))
// 									&& (0 < dot_v2(AMstart, AD) && dot_v2(AMstart, AD) < dot_v2 (AD, AD));

// 			bool32 endInside = 	(0 < dot_v2(AMend, AB) && dot_v2(AMend, AB) < dot_v2 (AB, AB))
// 								&& (0 < dot_v2(AMend, AD) && dot_v2(AMend, AD) < dot_v2 (AD, AD));

// 			if (!startInside && endInside)
// 				return true;
// 		}

// 		return false;
// 	}

// 	void
// 	do_collisions() 
// 	{
// 		// Reset previous collisions
// 		// flush_arena_array(collisions);
// 		collisions.flush();

// 		for (s32 i = 0; i < colliders.count(); ++i)
// 		{
// 			colliders[i].hasCollision = false;
// 			colliders[i].collision = nullptr;
// 		}

// 		// Calculate new collisions
// 		for (s32 a = 0; a < colliders.count(); ++a)
// 		{
// 			for (s32 b = a + 1; b < colliders.count(); ++b)
// 			{
// 				v2 positionA = v2{colliders[a].transform->position.x, colliders[a].transform->position.z} + colliders[a].offset;
// 				v2 positionB = v2{colliders[b].transform->position.x, colliders[b].transform->position.z} + colliders[b].offset;

// 				float xDeltaAtoB 	= positionB.x - positionA.x;
// 				float xDistance		= abs_f32(xDeltaAtoB);

// 				float xRadiusA = colliders[a].extents.x;
// 				float xRadiusB = colliders[b].extents.x;
// 				float xLimit = xRadiusA + xRadiusB;

// 				bool32 xCollision = xDistance < xLimit; 

// 				float zDeltaAtoB 	= positionB.y - positionA.y;
// 				float zDistance		= abs_f32(zDeltaAtoB);

// 				float zRadiusA = colliders[a].extents.y;
// 				float zRadiusB = colliders[b].extents.y;
// 				float zLimit = zRadiusA + zRadiusB;

// 				bool32 zCollision = zDistance < zLimit; 

// 				if (xCollision && zCollision)
// 				{
// 					colliders[a].hasCollision = true;
// 					colliders[b].hasCollision = true;

// 					float xNormalA = sign_f32(xDeltaAtoB);
// 					float xNormalB = -xNormalA;

// 					u64 collisionIndexForA = collisions.count();
// 					collisions.push({
// 						.position 	= positionB.x + xNormalB * xRadiusB,
// 						.normal 	= {xNormalB, 0, 0},
// 						.tag 		= colliders[b].tag
// 					});
// 					colliders[a].collision = &collisions[collisionIndexForA];

// 					u64 collisionIndexForB = collisions.count();
// 					collisions.push({
// 						.position 	= positionA.x + xNormalA * xRadiusA, 
// 						.normal 	= {xNormalA, 0, 0},
// 						.tag 		= colliders[a].tag
// 					});
// 					colliders[b].collision = &collisions[collisionIndexForB];
// 				}
// 			}
// 		}
// 	}
// };


// internal Collider2D *
// push_collider(	CollisionManager2D * 	manager,
// 				Transform3D * 			transform,
// 				v2 				extents,
// 				v2 				offset = {0, 0},
// 				ColliderTag 			tag = ColliderTag::Default)
// {
// 	u64 index = manager->colliders.count();
// 	manager->colliders.push(
// 	{
// 		.transform 	= transform,
// 		.extents 	= extents,
// 		.offset		= offset,
// 		.tag 		= tag
// 	});
// 	return &manager->colliders[index];

// 	// auto collider = make_handle<Collider2D>({
// 	// 	.transform 	= transform,
// 	// 	.extents 	= extents,
// 	// 	.offset		= offset,
// 	// 	.tag 		= tag
// 	// });
// 	// push_one(manager->colliders, collider);
// 	// return collider;
// }
