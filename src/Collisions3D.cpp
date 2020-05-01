struct BoxCollider
{
	v3 extents;
	v3 center; 

	Transform3D * transform;
};

struct CylinderCollider
{
	f32 radius;
	f32 height;
	v3 center;

	Transform3D * transform;
};

struct CollisionSystem3D
{
	Array<BoxCollider> 		boxColliders;
	Array<CylinderCollider> cylinderColliders;

	// Todo(Leo): include these to above
	HeightMap terrainCollider;
	Transform3D * terrainTransform;
};


internal void push_box_collider (	CollisionSystem3D & system,
									v3 					extents,
									v3 					center,
									Transform3D * 		transform)
{
	system.boxColliders.push({extents, center, transform});
}


internal void push_cylinder_collider ( 	CollisionSystem3D & system,
										f32 radius,
										f32 height,
										v3 center,
										Transform3D * transform)
{
	system.cylinderColliders.push({radius, height, center, transform});
}

internal float
get_terrain_height(CollisionSystem3D * system, v2 position)
{
	position.x -= system->terrainTransform->position.x;
	position.y -= system->terrainTransform->position.y;
	float value = get_height_at(&system->terrainCollider, position);
	return value;
}

struct RaycastResult
{
	v3 hitPosition;
	v3 hitNormal;
};

internal bool32
raycast_3d(	CollisionSystem3D * system,
			v3 rayStart,
			v3 normalizedRayDirection,
			float rayLength,
			RaycastResult * outResult = nullptr)
{
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

	for (auto const & collider : system->boxColliders)
	{
		v3 position = collider.transform->position + collider.center;

		v3 min = position - collider.extents;
		v3 max = position + collider.extents;

		v3 inverseDirection = 
		{
			1.0f / normalizedRayDirection.x,
			1.0f / normalizedRayDirection.y,
			1.0f / normalizedRayDirection.z,
		};

		float xDistanceToMin = (min.x - rayStart.x) * inverseDirection.x;
		float xDistanceToMax = (max.x - rayStart.x) * inverseDirection.x;
		if (inverseDirection.x < 0)
		{
			swap(xDistanceToMin, xDistanceToMax);
		}

		float yDistanceToMin = (min.y - rayStart.y) * inverseDirection.y;
		float yDistanceToMax = (max.y - rayStart.y) * inverseDirection.y;
		if (inverseDirection.y < 0)
		{
			swap(yDistanceToMin, yDistanceToMax);
		}
		
		float zDistanceToMin = (min.z - rayStart.z) * inverseDirection.z;
		float zDistanceToMax = (max.z - rayStart.z) * inverseDirection.z;
		if (inverseDirection.z < 0)
		{
			swap(zDistanceToMin, zDistanceToMax);
		}

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

		if (0.00001f < distanceToMin && distanceToMin < rayLength)
		{

			// Note(Leo): This is only supporting axis-aligned colliders
			if (outResult != nullptr)
			{
				// Note(Leo): Find maximum since biggest af smallest is the distance of hit
				if (xDistanceToMin > yDistanceToMin && xDistanceToMin > zDistanceToMin)
				{
					outResult->hitNormal = {-Sign(normalizedRayDirection.x), 0, 0};
				}
				else if (yDistanceToMin > zDistanceToMin)
				{
					outResult->hitNormal = {0, -Sign(normalizedRayDirection.y), 0};
				}
				else
				{
					outResult->hitNormal = {0, 0, -Sign(normalizedRayDirection.z)};
				}

				outResult->hitPosition = rayStart + normalizedRayDirection * distanceToMin;
			}


			return true;
		}
	}

	for (auto const & collider : system->cylinderColliders)
	{
		/*
			test z position against height
			test xy distance againts radius
		*/

		v2 p = *rayStart.xy();
		v2 d = *normalizedRayDirection.xy();
		f32 dMagnitude = magnitude(d);
		d = normalize(d);

		v3 cPosition = collider.transform->position + collider.center;

		v2 toCircleCenter 				= *cPosition.xy() - p;

		if (dot_product(d, toCircleCenter) < 0.0f)
		{
			continue;
		}

		v2 projectionToCircleCenter = dot_product(d, toCircleCenter) * d;
		f32 projectionLength		= dot_product(d, toCircleCenter);

		v2 rejectionToCircleCenter 	= toCircleCenter - projectionToCircleCenter;
		f32 rejectionLength 		= magnitude(rejectionToCircleCenter);

		// Note(Leo): sqrBDistance < 0 and xyFar < xyNear both rule that tangent is
		// counted as hit, we'd rathrer not, but it does not matter since it is so thin

		using namespace math;

		f32 sqrBDistance = pow2(collider.radius) - pow2(rejectionLength);
		if (sqrBDistance < 0)
		{
			continue;
		}	

		f32 bDistance = square_root(sqrBDistance);
		// f32 bDistance = square_root(pow2(collider.radius) - pow2(rejectionLength));

		f32 xyNear = (projectionLength - bDistance) / dMagnitude;
		f32 xyFar = (projectionLength + bDistance) / dMagnitude;

		if (xyFar < xyNear)
		{
			continue;
		}

		if(xyNear > rayLength)
		{
			continue;
		}

		// ---------------------------------

		f32 zMin = (cPosition.z - collider.height / 2 - rayStart.z) / normalizedRayDirection.z;
		f32 zMax = (cPosition.z + collider.height / 2 - rayStart.z) / normalizedRayDirection.z;

		f32 zNear, zFar;
		if(zMin < zMax)
		{
			zNear = zMin;
			zFar = zMax;
		}
		else
		{
			zNear = zMax;
			zFar = zMin;
		}

		// ----------------------------------

		f32 near = max(xyNear, zNear);
		f32 far  = min(xyFar, zFar);

		if (far < near)
		{
			continue;
		}

		if (near > rayLength)
		{
			continue;
		}


		v2 hitNormal2 	= -rejectionToCircleCenter + (-bDistance * d);
		v3 hitNormal 	= {hitNormal2.x, hitNormal2.y, 0};

		outResult->hitPosition = cPosition + hitNormal;
		outResult->hitNormal = normalize(hitNormal);

		return true;
	}

	return false;
}