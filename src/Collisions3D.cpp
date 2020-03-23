struct BoxCollider3D
{
	v3 extents;
	v3 center; 
};

struct CollisionSystemEntry
{
	BoxCollider3D * collider;
	Transform3D * transform;
};

struct CollisionSystem3D
{
	BETTERArray<CollisionSystemEntry> entries_;
	BETTERArray<BoxCollider3D> colliders_;

	// Todo(Leo): include these to above
	HeightMap terrainCollider;
	Transform3D * terrainTransform;
};

internal void
allocate_collision_system(CollisionSystem3D * system, MemoryArena * memoryArena, u32 count)
{
	system->entries_ = allocate_BETTER_array<CollisionSystemEntry>(*memoryArena, count); 
	system->colliders_ = allocate_BETTER_array<BoxCollider3D>(*memoryArena, count);
}

internal void
push_collider_to_system(CollisionSystem3D * system,
						BoxCollider3D  		collider,
						Transform3D *		transform)
{
	auto index = system->colliders_.count();
	system->colliders_.push(collider);
	system->entries_.push({&system->colliders_[index], transform});
}

internal float
get_terrain_height(CollisionSystem3D * system, vector2 position)
{
	position.x -= system->terrainTransform->position.x;
	position.y -= system->terrainTransform->position.y;
	float value = get_height_at(&system->terrainCollider, position);
	// std::cout << "[get_terrain_height()]: value = " << value << "\n";
	return value;
}

struct RaycastResult
{
	v3 hitNormal;
};

internal bool32
raycast_3d(	CollisionSystem3D * manager,
			v3 rayStart,
			v3 normalizedRayDirection,
			float rayLength,
			RaycastResult * outResult = nullptr)
{
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

	for (auto entry : manager->entries_)
	{
		auto collider = entry.collider;
		auto transform = entry.transform;

		v3 position = 	get_world_position(*transform) + collider->center;

		v3 min = position - collider->extents;
		v3 max = position + collider->extents;

		v3 inverseDirection = 
		{
			1.0f / normalizedRayDirection.x,
			1.0f / normalizedRayDirection.y,
			1.0f / normalizedRayDirection.z,
		};

		auto swap_if = [](float * a, float * b, bool condition)
		{
			if (condition == false)
				return;

			float temp = *a;
			*a = *b;
			*b = temp;
		};

		float xDistanceToMin = (min.x - rayStart.x) * inverseDirection.x;
		float xDistanceToMax = (max.x - rayStart.x) * inverseDirection.x;
		swap_if(&xDistanceToMin, &xDistanceToMax, inverseDirection.x < 0);

		float yDistanceToMin = (min.y - rayStart.y) * inverseDirection.y;
		float yDistanceToMax = (max.y - rayStart.y) * inverseDirection.y;
		swap_if(&yDistanceToMin, &yDistanceToMax, inverseDirection.y < 0);
		
		float zDistanceToMin = (min.z - rayStart.z) * inverseDirection.z;
		float zDistanceToMax = (max.z - rayStart.z) * inverseDirection.z;
		swap_if(&zDistanceToMin, &zDistanceToMax, inverseDirection.z < 0);

		// Note(Leo): if any min distance is more than any max distance
		if ((xDistanceToMin > yDistanceToMax) || (yDistanceToMin > xDistanceToMax))
		{
			// no collision
			continue;
		}			
	

		float distanceToMin = math::max(xDistanceToMin, yDistanceToMin);
		float distanceToMax = math::min(xDistanceToMax, yDistanceToMax);
		

		if ((distanceToMin > zDistanceToMax) || (zDistanceToMin > distanceToMax))
		{
			// no collision
			continue;
		}			

		distanceToMin = math::max(distanceToMin, zDistanceToMin);
		distanceToMax = math::min(distanceToMax, zDistanceToMax);

		// Todo(Leo): make and use global epsilon
		if (distanceToMin > 0.0f && distanceToMin < rayLength)
		{
			if (outResult != nullptr)
			{
				if (xDistanceToMin < yDistanceToMin && xDistanceToMin < zDistanceToMin)
				{
					outResult->hitNormal = {-Sign(normalizedRayDirection.x), 0, 0};
				}
				else if (yDistanceToMin < zDistanceToMin)
				{
					outResult->hitNormal = {0, -Sign(normalizedRayDirection.y), 0};
				}
				else
				{
					outResult->hitNormal = {0, 0, -Sign(normalizedRayDirection.z)};
				}
			}


			return true;
		}

	}
	return false;
}