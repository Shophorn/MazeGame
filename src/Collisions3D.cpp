struct BoxCollider3D
{
	Vector3 extents;
	Vector3 center; 
};

struct CollisionSystemEntry
{
	Handle<BoxCollider3D> collider = {};
	Handle<Transform3D> transform = {};
};

struct CollisionSystem3D
{
	ArenaArray<CollisionSystemEntry> colliders = {};
};

internal void
push_collider_to_system(CollisionSystem3D * 	system,
						Handle<BoxCollider3D> 	collider,
						Handle<Transform3D>		transform)
{
	push_one(system->colliders, {collider, transform});
}

internal bool
raycast_3d(	CollisionSystem3D * manager,
			Vector3 rayStart,
			Vector3 normalizedRayDirection,
			float rayLength)
{
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

	for (auto entry : manager->colliders)
	{
		auto collider = entry.collider;
		auto transform = entry.transform;

		Vector3 position = 	transform->get_world_position()
							+ collider->center;

		Vector3 min = position - collider->extents;
		Vector3 max = position + collider->extents;

		Vector3 inverseDirection = 
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
		
		float distanceToMin = Max(xDistanceToMin, yDistanceToMin);
		float distanceToMax = Min(xDistanceToMax, yDistanceToMax);

		if ((distanceToMin > zDistanceToMax) || (zDistanceToMin > distanceToMax))
		{
			// no collision
			continue;
		}			

		distanceToMin = Max(distanceToMin, zDistanceToMin);
		distanceToMax = Min(distanceToMax, zDistanceToMax);

		// Todo(Leo): make and use global epsilon
		if (distanceToMin > 0.0f && distanceToMin < rayLength)
		{
			return true;
		}

	}
	return false;
}