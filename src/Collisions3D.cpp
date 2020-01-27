struct BoxCollider3D
{
	vector3 extents;
	vector3 center; 
};

struct CollisionSystemEntry
{
	Handle<BoxCollider3D> collider;
	Handle<Transform3D> transform;
};

struct CollisionSystem3D
{
	ArenaArray<CollisionSystemEntry> colliders = {};

	HeightMap terrainCollider;
	Handle<Transform3D> terrainTransform;
};

internal void
push_collider_to_system(CollisionSystem3D * 	system,
						Handle<BoxCollider3D> 	collider,
						Handle<Transform3D>		transform)
{
	push_one(system->colliders, {collider, transform});
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
	vector3 hitNormal;
};

internal bool32
raycast_3d(	CollisionSystem3D * manager,
			vector3 rayStart,
			vector3 normalizedRayDirection,
			float rayLength,
			RaycastResult * outResult = nullptr)
{
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

	for (auto entry : manager->colliders)
	{
		auto collider = entry.collider;
		auto transform = entry.transform;

		vector3 position = 	get_world_position(transform) + collider->center;

		vector3 min = position - collider->extents;
		vector3 max = position + collider->extents;

		vector3 inverseDirection = 
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