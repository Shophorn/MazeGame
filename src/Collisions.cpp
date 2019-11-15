struct Rectangle
{
	Vector2 position;
	Vector2 size;
};

struct Circle
{
	Vector2 position;
	real32 radius;
};

internal bool32
CircleCircleCollision(Circle a, Circle b)
{
	real32 distanceThreshold = a.radius + b.radius;
	real32 sqrDistanceThreshold = distanceThreshold * distanceThreshold;

	real32 sqrDistance = SqrLength (a.position - b.position);

	bool32 result = sqrDistance < sqrDistanceThreshold;
	return result;
}

struct CollisionResult
{
	bool32 isCollision;
	int32 otherColliderIndex;
};

internal CollisionResult
GetCollisions(Circle collider, ArenaArray<Circle> otherColliders)
{
	for (int colliderIndex = 0; colliderIndex < otherColliders.count; ++colliderIndex)
	{
		if (CircleCircleCollision(collider, otherColliders[colliderIndex]))
		{
			return {true, colliderIndex};
		}
	}
	return {false};
}