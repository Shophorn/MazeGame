struct BoxCollider
{
	v3 			extents;
	v3 			center; 
	quaternion 	orientation;

	Transform3D * transform;
};

struct RaycastResult
{
	v3 hitPosition;
	v3 hitNormal;
};

struct CylinderCollider
{
	f32 radius;
	f32 height;
	v3 center;

	Transform3D * transform;
};

struct StaticBoxCollider
{
	m44 transform;
	m44 inverseTransform;
};

struct CollisionSystem3D
{
	Array<BoxCollider> 	boxColliders;
	Array<CylinderCollider> cylinderColliders;

	Array<StaticBoxCollider> staticBoxColliders;
	Array<StaticBoxCollider> precomputedColliders;

	// Todo(Leo): include these to above
	HeightMap terrainCollider;
	Transform3D * terrainTransform;
};

internal m44 compute_collider_transform(BoxCollider const & collider)
{
	m44 transformMatrix = 	translation_matrix(collider.transform->position)
							* rotation_matrix(collider.transform->rotation)
							* translation_matrix(collider.center)
							* rotation_matrix(collider.orientation)
							* scale_matrix(collider.extents);
	return transformMatrix;
}

internal m44 compute_inverse_collider_transform(BoxCollider const & collider)
{
	v3 inverseScale = { 1.0f / collider.extents.x,
						1.0f / collider.extents.y,
						1.0f / collider.extents.z };

	m44 inverseColliderTransform = 	scale_matrix(inverseScale)
									* rotation_matrix(inverse_quaternion(collider.orientation))
									* translation_matrix(-collider.center)
									* rotation_matrix(inverse_quaternion(collider.transform->rotation))
		 							* translation_matrix(-collider.transform->position);

	return inverseColliderTransform;
}

internal void precompute_colliders(CollisionSystem3D & system)
{
	s32 boxColliderCount = system.boxColliders.count();
	clear_array(system.precomputedColliders);
	system.precomputedColliders = allocate_array<StaticBoxCollider>(*global_transientMemory, boxColliderCount, ALLOC_NO_CLEAR | ALLOC_FILL);

	for (s32 i = 0; i < boxColliderCount; ++i)
	{
		const BoxCollider & collider = system.boxColliders[i];

		system.precomputedColliders[i].transform 		= compute_collider_transform(collider);
		system.precomputedColliders[i].inverseTransform = compute_inverse_collider_transform(collider);
	}
}

internal void push_box_collider (	CollisionSystem3D & system,
									v3 					extents,
									v3 					center,
									Transform3D * 		transform)
{
	system.boxColliders.push({extents, center, identity_quaternion, transform});
}


internal void push_cylinder_collider ( 	CollisionSystem3D & system,
										f32 radius,
										f32 height,
										v3 center,
										Transform3D * transform)
{
	system.cylinderColliders.push({radius, height, center, transform});
}

internal f32
get_terrain_height(CollisionSystem3D * system, v2 position)
{
	position.x -= system->terrainTransform->position.x;
	position.y -= system->terrainTransform->position.y;
	f32 value = get_height_at(&system->terrainCollider, position);
	return value;
}

internal bool32 ray_box_collisions(	Array<StaticBoxCollider> & colliders,
									v3 rayStart,
									v3 normalizedRayDirection,
									f32 rayLength,
									RaycastResult * outResult,
									f32 * rayHitSquareDistance)
{
	bool32 hit = false;

	for (auto const & collider : colliders)
	{
		v3 colliderSpaceRayStart 		= multiply_point(collider.inverseTransform, rayStart);
		v3 colliderSpaceRayDirection 	= multiply_direction(collider.inverseTransform, normalizedRayDirection);  

		// Study(Leo): this seems to work, but I still have concerns
		f32 colliderSpaceRayLength = rayLength;

		v3 min = {-1,-1,-1};
		v3 max = {1,1,1};

		v3 inverseDirection = 
		{
			1.0f / colliderSpaceRayDirection.x,
			1.0f / colliderSpaceRayDirection.y,
			1.0f / colliderSpaceRayDirection.z,
		};


		// Todo(Leo): remove swaps, just put assignements directly to if blocks
		f32 xDistanceToMin = (min.x - colliderSpaceRayStart.x) * inverseDirection.x;
		f32 xDistanceToMax = (max.x - colliderSpaceRayStart.x) * inverseDirection.x;
		if (inverseDirection.x < 0)
		{
			swap(xDistanceToMin, xDistanceToMax);
		}

		f32 yDistanceToMin = (min.y - colliderSpaceRayStart.y) * inverseDirection.y;
		f32 yDistanceToMax = (max.y - colliderSpaceRayStart.y) * inverseDirection.y;
		if (inverseDirection.y < 0)
		{
			swap(yDistanceToMin, yDistanceToMax);
		}
		
		f32 zDistanceToMin = (min.z - colliderSpaceRayStart.z) * inverseDirection.z;
		f32 zDistanceToMax = (max.z - colliderSpaceRayStart.z) * inverseDirection.z;
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

		f32 distanceToMin = math::max(xDistanceToMin, yDistanceToMin);
		f32 distanceToMax = math::min(xDistanceToMax, yDistanceToMax);
		

		if ((distanceToMin > zDistanceToMax) || (zDistanceToMin > distanceToMax))
		{
			// no collision
			continue;
		}			

		distanceToMin = math::max(distanceToMin, zDistanceToMin);
		distanceToMax = math::min(distanceToMax, zDistanceToMax);

		if (0.00001f < distanceToMin && distanceToMin < colliderSpaceRayLength)
		{
			hit = true;

			v3 colliderSpaceHitPosition = colliderSpaceRayStart + colliderSpaceRayDirection * distanceToMin;
			v3 hitPosition = multiply_point(collider.transform, colliderSpaceHitPosition);
			f32 sqrDistance = square_magnitude_v3(hitPosition - rayStart);

			if (sqrDistance < *rayHitSquareDistance)
			{
				*rayHitSquareDistance = sqrDistance;

				// Note(Leo): This is only supporting axis-aligned colliders
				if (outResult != nullptr)
				{
					// Note(Leo): Find maximum since biggest af smallest is the distance of hit
					if (xDistanceToMin > yDistanceToMin && xDistanceToMin > zDistanceToMin)
					{
						outResult->hitNormal = {-Sign(colliderSpaceRayDirection.x), 0, 0};
					}
					else if (yDistanceToMin > zDistanceToMin)
					{
						outResult->hitNormal = {0, -Sign(colliderSpaceRayDirection.y), 0};
					}
					else
					{
						outResult->hitNormal = {0, 0, -Sign(colliderSpaceRayDirection.z)};
					}

					outResult->hitPosition 	= 	colliderSpaceRayStart
												+ colliderSpaceRayDirection
												* distanceToMin;

					outResult->hitPosition = multiply_point(collider.transform, outResult->hitPosition);

					outResult->hitNormal = multiply_direction(collider.transform, outResult->hitNormal);
					outResult->hitNormal = normalize_v3(outResult->hitNormal);//*= colliderSpaceRayLengthMultiplier;
				}

			}

			// f32 sqrDistance = square_magnitude_v3(outResult->hitPosition, rayStart);
			// if (sqr)
			// rayHitSquareDistance = 
			// // return true;
		}
	}
	return hit;
}

internal bool32
raycast_3d(	CollisionSystem3D * system,
			v3 rayStart,
			v3 normalizedRayDirection,
			f32 rayLength,
			RaycastResult * outResult = nullptr)
{
	// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection

	f32 rayHitSquareDistance 	= highest_f32;
	bool32 hit 					= false;

	hit = hit | ray_box_collisions(system->precomputedColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);
	hit = hit | ray_box_collisions(system->staticBoxColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);


	for (auto const & collider : system->cylinderColliders)
	{
		using namespace math;

		v3 const center = collider.transform->position + collider.center;
		v3 const p = rayStart - center;
		v3 const v = normalizedRayDirection;
		f32 const halfHeight = collider.height / 2;

#if 0
		f32 vt0 = (halfHeight - p.z) / v.z;
		f32 vt1 = (-halfHeight - p.z) / v.z;

		f32 vtMin = math::min(vt0, vt1);

		if (vtMin > 0.00001f && vtMin < rayLength)
		{
			v3 pAtVttMin = p + v * vtMin;
			f32 distanceFromCenter = magnitude_v2(pAtVttMin.xy);
			if (distanceFromCenter < collider.radius)
			{
				hit = true;

				outResult->hitPosition = pAtVttMin + center;
				outResult->hitNormal = {0,0,1};
			}
		}
#else
		f32 vt0 = (halfHeight - p.z) / v.z;
		f32 vt1 = (-halfHeight - p.z) / v.z;

		f32 vtMin = math::min(vt0, vt1);

		if (vtMin > 0.00001f && vtMin < rayLength)
		{
			v3 pAtVttMin = p + v * vtMin;
			f32 distanceFromCenter = magnitude_v2(pAtVttMin.xy);
			if (distanceFromCenter < collider.radius)
			{
				hit = true;

				outResult->hitPosition = pAtVttMin + center;
				outResult->hitNormal = {0,0,1};
			}
		}


		// Quadratic form for ray stuff
		// Note(Leo): these seem like dot products
		f32 a = pow2(v.x) + pow2(v.y);
		f32 b = 2 * (p.x * v.x + p.y * v.y);
		f32 c = pow2(p.x) + pow2(p.y);


		if (math::absolute(a) < 0.00001f)
		{
			continue;
		}

		f32 r2 = math::pow2(collider.radius);
		// We are looking solutions for r2, not 0
		f32 cr2 = c - r2;

		// discriminant
		f32 D = b * b - 4 * a * cr2;

		// No roots no collisions
		// Also one root, no collision
		if (D < 0.00001f)
		{
			continue;
		}

		f32 sqrtD = math::square_root(D);
		f32 ht0 = (-b - sqrtD) / (2 * a);
		f32 ht1 = (-b + sqrtD) / (2 * a);

		f32 ht = math::min(ht0, ht1);

		if (ht > 0.00001f && ht < rayLength)
		{
			v3 pAtHtMin = p + v * ht;
			if (pAtHtMin.z > -halfHeight && pAtHtMin.z < halfHeight)
			{
				hit = true;
				if (rayHitSquareDistance > ht*ht)
				{
					rayHitSquareDistance = ht*ht;

					outResult->hitPosition = pAtHtMin + center;

					// Note(Leo): Hitposition - center, is weird
					v3 hitNormal = p + v * ht;
					hitNormal.z = 0;
					hitNormal = normalize_v3(hitNormal);
					outResult->hitNormal = hitNormal;
				}
			}
		}

#endif

		// v3 hit0 = rayStart + normalizedRayDirection * t0;
		// v3 hit1 = rayStart + normalizedRayDirection * t1;



		// /// cylinder ray collision, consider xy plane first intesrecting a triangle, and height separately

		// using namespace math;

		// // Todo(Leo): This can be implemented with 3d vectors with better understanding of geometry
		// // Hint: Lots of dot products


		// // Todo(Leo): name properly, these match to names in my notebook
		// v2 d = normalizedRayDirection.xy;
		// f32 horizontalRayLength = rayLength * magnitude_v2(d);
		// v2 C = collider.transform->position.xy;
		// v2 R = rayStart.xy;
		// v2 l = C - R;
		// v2 x = d * dot_v2(l, d);
		// v2 P = R + x;
		// v2 cp = P - C;
		// f32 cpLength = magnitude_v2(cp);
		// if (cpLength > collider.radius)
		// {
		// 	continue;
		// }
		// f32 nLength = square_root(collider.radius * collider.radius - cpLength * cpLength);
		
		// // Todo(Leo): Also check if nLength is longer than x, that is when we are inside
		// v2 H = P - d * nLength;

		// f32 hDistance = magnitude_v2(H - R);

		// f32 hDistanceToMin = magnitude_v2(H - R);
		// f32 hDistanceToMax = magnitude_v2((P + d * nLength) - R);//hDistance + 2 * math::absolute(nLength);

		// // if (hDistanceToMin > 0.00001f && hDistanceToMin < rayLength)
		// // {
		// // 	hit = true;

		// // 	outResult->hitPosition = rayStart + normalizedRayDirection * hDistanceToMin;
		// // 	outResult->hitNormal = {};
		// // 	outResult->hitNormal.xy = normalize_v2(outResult->hitPosition.xy - collider.transform->position.xy);

		// // 	rayHitSquareDistance = pow2(hDistanceToMin);
		// // }




		// /// ----------------------------------------------------------------------------

		// f32 vMin = collider.transform->position.z + collider.center.z - collider.height / 2;
		// f32 vMax = collider.transform->position.z + collider.center.z + collider.height / 2;

		// f32 vDistanceToMin = (vMin - rayStart.z) * (1.0f / normalizedRayDirection.z);
		// f32 vDistanceToMax = (vMax - rayStart.z) * (1.0f / normalizedRayDirection.z);
		// if ((1.0f / normalizedRayDirection.z) < 0)
		// {
		// 	swap(vDistanceToMin, vDistanceToMax);
		// }

		// // Note(Leo): if any min distance is more than any max distance
		// if ((hDistanceToMin > vDistanceToMax) || (vDistanceToMin > hDistanceToMax))
		// {
		// 	// no collision
		// 	continue;
		// }			

		// // f32 distanceToMin = math::max(hDistanceToMin, vDistanceToMin);
		// // f32 distanceToMax = math::min(hDistanceToMax, vDistanceToMax);

		



		// // if (normalizedRayDirection.z < -0.9999f)
		// if (normalizedRayDirection.z < -0.1f)
		// {
		// 	v3 circleCenter = collider.transform->position + collider.center;
		// 	f32 horizontalDistance = magnitude_v2(rayStart.xy - circleCenter.xy);

		// 	f32 colliderTopEdgeHeight = collider.transform->position.z + collider.center.z + (collider.height / 2.0f);
		// 	f32 verticalDistance = rayStart.z - colliderTopEdgeHeight;

		// 	bool32 horizontalDistanceOk = horizontalDistance < collider.radius;
		// 	bool32 verticalDistanceMoreThanZero = verticalDistance > 0.00001f;
		// 	bool32 verticalDistanceLessThanRay = verticalDistance < rayLength;

		// 	// if (horizontalDistance < collider.radius && verticalDistance > 0.00001f && verticalDistance < rayLength)
		// 	if (horizontalDistanceOk && verticalDistanceMoreThanZero && verticalDistanceLessThanRay)
		// 	{
		// 		hit = true;

		// 		outResult->hitPosition = {rayStart.x, rayStart.y, colliderTopEdgeHeight}
		// 		outResult->hitNormal = {0,0,1};

		// 		rayHitSquareDistance = pow2(verticalDistance);
		// 	}
		// }

		// else if (pow2(normalizedRayDirection.z) < 0.00001f)
		// {
		// 	if (hDistanceToMin > 0.00001f && hDistanceToMin < rayLength)
		// 	{
		// 		hit = true;

		// 		if (rayHitSquareDistance > pow2(hDistanceToMin))
		// 		{
		// 			outResult->hitPosition = rayStart + normalizedRayDirection * hDistanceToMin;
		// 			outResult->hitNormal = {};
		// 			outResult->hitNormal.xy = normalize_v2(outResult->hitPosition.xy - collider.transform->position.xy);

		// 			rayHitSquareDistance = pow2(hDistanceToMin);
		// 		}
		// 	}

		// }
		// else
		// {
		// 	Assert(false && "I do not know geometry, so no rays of z other than -1 or 0 allowed >:(");
		// }


		// continue;




		// if (hDistanceToMin > horizontalRayLength)
		// // if (hDistance > horizontalRayLength)
		// {
		// 	continue;
		// }

		// f32 verticalRaylength = normalizedRayDirection.z * rayLength;
		// f32 s = rayStart.z;
		// f32 p = collider.transform->position.z + collider.center.z + collider.height / 2;
		// f32 vDistance = s - p;

		// if (verticalRaylength < vDistance)
		// {
		// 	continue;
		// }


		// hit = true;

		// 	if (square_magnitude_v2(H - R) < rayHitSquareDistance)
		// 	{
		// 		rayHitSquareDistance = square_magnitude_v2(H - R);

		// 		v3 hitPosition = {H.x, H.y, rayStart.z};

		// 		v2 xyHitNormal = normalize_v2(H - C);
		// 		v3 hitNormal = {xyHitNormal.x, xyHitNormal.y, 0};	

		// 		outResult->hitPosition = hitPosition;
		// 		outResult->hitNormal = hitNormal;
		// 	}
	

		// // v2 circleCenter = collider.transform->position.xy;// + collider.center.xy;		

		// // v2 rayStartToCircleCenter = circleCenter - rayStart.xy;

		// // v2 xyDirection 			= normalize_v2(normalizedRayDirection.xy);
		// // v2 rayToCenterPassPoint = xyDirection * dot_v2(rayStartToCircleCenter, xyDirection);

		// // v2 centerToPassPoint 	= rayToCenterPassPoint - rayStartToCircleCenter;
		// // f32 centerToPassPointLength = magnitude_v2(centerToPassPoint);
		// // if (centerToPassPointLength > collider.radius)
		// // {
		// // 	// No collision
		// // 	continue;
		// // }

		// // // f32 nagle = arc_cosine(centerToPassPointLength / collider.radius);

		// // // f32 passPointToHitPointLength = sine(nagle);
		// // f32 passPointToHitPointLength = math::square_root(math::pow2(collider.radius) - math::pow2(centerToPassPointLength));


		// // v2 passPointToHitPoint = -xyDirection * passPointToHitPointLength;

		// // v2 xyHitPoint = rayStart.xy + rayToCenterPassPoint + passPointToHitPoint;
		// // v2 xyHitNormal = normalize_v2(xyHitPoint - circleCenter);

		// // v3 hitPosition = { xyHitPoint.x, xyHitPoint.y, rayStart.z };
		// // v3 hitNormal = { xyHitNormal.x, xyHitNormal.y, 0 };


		// // f32 sqrDistance = square_magnitude_v3(hitPosition - rayStart);

		// // if (math::square_root(sqrDistance) < rayLength)
		// // {
		// // 	hit = true;
		// // 	if (sqrDistance < rayHitSquareDistance)
		// // 	{
		// // 		rayHitSquareDistance 	= sqrDistance;
		// // 		outResult->hitPosition 	= hitPosition;
		// // 		outResult->hitNormal 	= hitNormal;
		// // 	}
		// // }





		// // /*
		// // 	xy projection on circle in position.xy along the rays direction on xy
			

		// // */







		// // /*
		// // 	test z position against height
		// // 	test xy distance againts radius
		// // */

		// // v2 p 			= rayStart.xy;
		// // v2 d 			= normalizedRayDirection.xy;
		// // f32 dMagnitude 	= magnitude_v2(d);
		// // if(dMagnitude > 0.00001f)
		// // {
		// // 	d = d / dMagnitude;
		// // }


		// // v3 cPosition = collider.transform->position + collider.center;

		// // v2 toCircleCenter = cPosition.xy - p;

		// // if (dot_v2(d, toCircleCenter) < 0.0f)
		// // {
		// // 	continue;
		// // }

		// // v2 projectionToCircleCenter = d * dot_v2(d, toCircleCenter);
		// // f32 projectionLength		= dot_v2(d, toCircleCenter);

		// // v2 rejectionToCircleCenter 	= toCircleCenter - projectionToCircleCenter;
		// // f32 rejectionLength 		= magnitude_v2(rejectionToCircleCenter);

		// // // Note(Leo): sqrBDistance < 0 and hFar < hNear both rule that tangent is
		// // // counted as hit, we'd rathrer not, but it does not matter since it is so thin

		// // using namespace math;

		// // f32 sqrBDistance = pow2(collider.radius) - pow2(rejectionLength);
		// // if (sqrBDistance < 0)
		// // {
		// // 	continue;
		// // }	

		// // f32 bDistance = square_root(sqrBDistance);
		// // // f32 bDistance = square_root(pow2(collider.radius) - pow2(rejectionLength));

		// // f32 hNear = (projectionLength - bDistance) / dMagnitude;
		// // f32 hFar = (projectionLength + bDistance) / dMagnitude;

		// // if (hFar < hNear)
		// // {
		// // 	continue;
		// // }

		// // if(hNear > rayLength)
		// // {
		// // 	continue;
		// // }

		// // // ---------------------------------

		// // f32 vMin = (cPosition.z - collider.height / 2 - rayStart.z) / normalizedRayDirection.z;
		// // f32 vMax = (cPosition.z + collider.height / 2 - rayStart.z) / normalizedRayDirection.z;

		// // f32 vNear, vFar;
		// // if(vMin < vMax)
		// // {
		// // 	vNear = vMin;
		// // 	vFar = vMax;
		// // }
		// // else
		// // {
		// // 	vNear = vMax;
		// // 	vFar = vMin;
		// // }

		// // // ----------------------------------

		// // f32 near = max(hNear, vNear);
		// // f32 far  = min(hFar, vFar);

		// // if (far < near)
		// // {
		// // 	continue;
		// // }

		// // if (near > rayLength)
		// // {
		// // 	continue;
		// // }

		// // v2 hitNormal2 	= -rejectionToCircleCenter + (d * -bDistance);
		// // v3 hitNormal 	= {hitNormal2.x, hitNormal2.y, 0};

		// // v3 hitPosition = cPosition + hitNormal;
		// // f32 sqrDistance = square_magnitude_v3(hitPosition - rayStart);
		// // if (sqrDistance < rayHitSquareDistance)
		// // {
		// // 	rayHitSquareDistance = sqrDistance;
			
		// // 	outResult->hitPosition = hitPosition;
		// // 	outResult->hitNormal = normalize_v3(hitNormal);
		// // }

		// // hit = true;
		// // // return true;
	}

	return hit;
	// return false;
}