struct Rectangle
{
	v2 position;
	v2 size;
};

internal Rectangle
scale_rectangle(Rectangle rect, v2 scale)
{
	v2 center = rect.position + rect.size / 2.0f;
	rect.size.x *= scale.x;
	rect.size.y *= scale.y;
	rect.position = center - rect.size / 2.0f;

	return rect;
}

struct Circle
{
	v2 position;
	float radius;
};

internal bool32
CircleCircleCollision(Circle a, Circle b)
{
	DEBUG_ASSERT(false, "vector::get_sqr_distance() and therefore this function too, is not tested");

	float distanceThreshold = a.radius + b.radius;
	float sqrDistanceThreshold = distanceThreshold * distanceThreshold;

	float sqrDistance = square_magnitude_v2(a.position - b.position);

	bool32 result = sqrDistance < sqrDistanceThreshold;
	return result;
}

internal bool32
CircleRectangleCollisionAABB(Circle c, Rectangle r)
{
	bool32 isInsideAABB = 
		(c.position.x + c.radius) > r.position.x
		&& (c.position.y + c.radius) > r.position.y
		&& (c.position.x - c.radius) < (r.position.x + r.size.x)
		&& (c.position.y - c.radius) < (r.position.y + r.size.y);

	float sqrDistanceThreshold = c.radius * c.radius;

	return isInsideAABB;
}

// struct CollisionResult
// {
// 	bool32 isCollision;
// 	s32 otherColliderIndex;
// };

// internal CollisionResult
// GetCollisions(Circle collider, Array<Circle> const & otherColliders)
// {
// 	for (int otherIndex = 0; otherIndex < otherColliders.count(); ++otherIndex)
// 	{
// 		if (CircleCircleCollision(collider, otherColliders[otherIndex]))
// 		{
// 			return {true, otherIndex};
// 		}
// 	}
// 	return {false};
// }

// internal CollisionResult
// GetCollisions(Circle collider, Array<Rectangle> const & otherColliders)
// {
// 	for (int otherIndex = 0; otherIndex < otherColliders.count(); ++otherIndex)
// 	{
// 		if (CircleRectangleCollisionAABB(collider, otherColliders[otherIndex]))
// 		{
// 			return { true, otherIndex };
// 		}
// 	}

// 	return {false};
// }