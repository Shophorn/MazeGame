struct BoxCollider
{
	v3 			extents;
	v3 			center; 
	quaternion 	orientation;

	Transform3D * transform;
};

struct Ray
{
	v3 start;
	v3 direction;	// Note(Leo): keep this always normalized
	f32 length;
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

struct BETTER_CylinderCollider
{
	f32 radius;
	f32 halfHeight;
	v3 center;
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
	Array<StaticBoxCollider> precomputedBoxColliders;

	Array<BETTER_CylinderCollider> precomputedCylinderColliders;
	Array<BETTER_CylinderCollider> submittedCylinderColliders;

	// Todo(Leo): include these to above
	HeightMap terrainCollider;
	Transform3D * terrainTransform;
};

internal m44 compute_collider_transform(BoxCollider const & collider)
{
	m44 transformMatrix = transform_matrix(collider.transform->position, collider.transform->rotation, {1,1,1});
	m44 colliderMatrix 	= transform_matrix(collider.center, collider.orientation, collider.extents);

	return transformMatrix * colliderMatrix;
}

internal m44 compute_inverse_collider_transform(BoxCollider const & collider)
{
	m44 colliderMatrix = inverse_transform_matrix(collider.center, collider.orientation, collider.extents);
	m44 transformMatrix = inverse_transform_matrix(collider.transform->position, collider.transform->rotation, {1,1,1});

	return colliderMatrix * transformMatrix;
}


internal void submit_cylinder_collider(CollisionSystem3D & system, f32 radius, f32 halfHeight, v3 center)
{
	system.submittedCylinderColliders.push({radius, halfHeight, center});
}

internal void precompute_colliders(CollisionSystem3D & system)
{
	s32 boxColliderCount = system.boxColliders.count();
	reset_array(system.precomputedBoxColliders);
	system.precomputedBoxColliders = allocate_array<StaticBoxCollider>(*global_transientMemory, boxColliderCount, ALLOC_NO_CLEAR | ALLOC_FILL);

	for (s32 i = 0; i < boxColliderCount; ++i)
	{
		const BoxCollider & collider = system.boxColliders[i];

		system.precomputedBoxColliders[i].transform 		= compute_collider_transform(collider);
		system.precomputedBoxColliders[i].inverseTransform = compute_inverse_collider_transform(collider);
	}

	// --------------------------------------------------------------------------------------------------

	s32 cylinderColliderCount = system.cylinderColliders.count();
	reset_array(system.precomputedCylinderColliders);
	system.precomputedCylinderColliders = allocate_array<BETTER_CylinderCollider>(*global_transientMemory, cylinderColliderCount, ALLOC_NO_CLEAR | ALLOC_FILL);

	for (s32 i = 0; i < cylinderColliderCount; ++i)
	{
		CylinderCollider const & collider = system.cylinderColliders[i];

		system.precomputedCylinderColliders[i].radius = collider.radius; 
		system.precomputedCylinderColliders[i].halfHeight = collider.height / 2; 
		system.precomputedCylinderColliders[i].center = collider.transform->position + collider.center; 
	}

	// --------------------------------------------------------------------------------------------------

	// Todo(Leo): this does not semantically belong here....
	reset_array(system.submittedCylinderColliders);
	system.submittedCylinderColliders = allocate_array<BETTER_CylinderCollider>(*global_transientMemory, 100, ALLOC_NO_CLEAR);
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

		f32 distanceToMin = max_f32(xDistanceToMin, yDistanceToMin);
		f32 distanceToMax = min_f32(xDistanceToMax, yDistanceToMax);
		

		if ((distanceToMin > zDistanceToMax) || (zDistanceToMin > distanceToMax))
		{
			// no collision
			continue;
		}			

		distanceToMin = max_f32(distanceToMin, zDistanceToMin);
		distanceToMax = min_f32(distanceToMax, zDistanceToMax);

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
						outResult->hitNormal = {-sign_f32(colliderSpaceRayDirection.x), 0, 0};
					}
					else if (yDistanceToMin > zDistanceToMin)
					{
						outResult->hitNormal = {0, -sign_f32(colliderSpaceRayDirection.y), 0};
					}
					else
					{
						outResult->hitNormal = {0, 0, -sign_f32(colliderSpaceRayDirection.z)};
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

internal bool32 ray_cylinder_collisions (s32 count, BETTER_CylinderCollider * colliders, Ray ray, RaycastResult * outResult, f32 * rayHitSquareDistance)
{
	bool32 hit = false;

	// for (auto const & collider : system->precomputedCylinderColliders)
	for (s32 i = 0; i < count; ++i)
	{
		auto & collider = colliders[i];

		v3 const center = collider.center;
		v3 const p = ray.start - center;
		v3 const v = ray.direction;
		f32 const halfHeight = collider.halfHeight;

		f32 vt0 = (halfHeight - p.z) / v.z;
		f32 vt1 = (-halfHeight - p.z) / v.z;

		f32 vtMin = min_f32(vt0, vt1);

		if (vtMin > 0.00001f && vtMin < ray.length)
		{
			v3 pAtVttMin = p + v * vtMin;
			f32 distanceFromCenter = magnitude_v2(pAtVttMin.xy);
			if (distanceFromCenter < collider.radius)
			{
				hit = true;

				if (*rayHitSquareDistance > vtMin * vtMin)
				{
					*rayHitSquareDistance = vtMin * vtMin;					

					outResult->hitPosition = pAtVttMin + center;
					outResult->hitNormal = {0,0,1};
				}
			}
		}


		// Quadratic form for ray stuff
		// Note(Leo): these seem like dot products
		f32 a = square_f32(v.x) + square_f32(v.y);
		f32 b = 2 * (p.x * v.x + p.y * v.y);
		f32 c = square_f32(p.x) + square_f32(p.y);


		// Undefined
		if (abs_f32(a) < 0.00001f)
		{
			continue;
		}

		// We are looking solutions for r2, not 0
		f32 cr2 = c - square_f32(collider.radius);

		// discriminant
		f32 D = b * b - 4 * a * cr2;

		// No roots no collisions
		// Also one root, no collision
		if (D < 0.00001f)
		{
			continue;
		}

		f32 sqrtD = square_root_f32(D);
		f32 ht0 = (-b - sqrtD) / (2 * a);
		f32 ht1 = (-b + sqrtD) / (2 * a);

		f32 ht = min_f32(ht0, ht1);

		if (ht > 0.00001f && ht < ray.length)
		{
			v3 pAtHtMin = p + v * ht;
			if (pAtHtMin.z > -halfHeight && pAtHtMin.z < halfHeight)
			{
				hit = true;
				if (*rayHitSquareDistance > ht*ht)
				{
					*rayHitSquareDistance = ht*ht;

					outResult->hitPosition = pAtHtMin + center;

					// Note(Leo): Hitposition - center, is weird
					v3 hitNormal = p + v * ht;
					hitNormal.z = 0;
					hitNormal = normalize_v3(hitNormal);
					outResult->hitNormal = hitNormal;
				}
			}
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

	hit = hit || ray_box_collisions(system->precomputedBoxColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);
	hit = hit || ray_box_collisions(system->staticBoxColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);

	hit = hit || ray_cylinder_collisions(	system->precomputedCylinderColliders.count(),
											system->precomputedCylinderColliders.data(),
											{rayStart, normalizedRayDirection, rayLength},
											outResult,
											&rayHitSquareDistance);

	hit = hit || ray_cylinder_collisions(	system->submittedCylinderColliders.count(),
											system->submittedCylinderColliders.data(),
											{rayStart, normalizedRayDirection, rayLength},
											outResult,
											&rayHitSquareDistance);

	// for (auto const & collider : system->precomputedCylinderColliders)
	// {
	// 	using namespace math;

	// 	v3 const center = collider.center;//collider.transform->position + collider.center;
	// 	v3 const p = rayStart - center;
	// 	v3 const v = normalizedRayDirection;
	// 	f32 const halfHeight = collider.halfHeight;

	// 	f32 vt0 = (halfHeight - p.z) / v.z;
	// 	f32 vt1 = (-halfHeight - p.z) / v.z;

	// 	f32 vtMin = math::min(vt0, vt1);

	// 	if (vtMin > 0.00001f && vtMin < rayLength)
	// 	{
	// 		v3 pAtVttMin = p + v * vtMin;
	// 		f32 distanceFromCenter = magnitude_v2(pAtVttMin.xy);
	// 		if (distanceFromCenter < collider.radius)
	// 		{
	// 			hit = true;

	// 			if (rayHitSquareDistance > vtMin * vtMin)
	// 			{
	// 				rayHitSquareDistance = vtMin * vtMin;					

	// 				outResult->hitPosition = pAtVttMin + center;
	// 				outResult->hitNormal = {0,0,1};
	// 			}
	// 		}
	// 	}


	// 	// Quadratic form for ray stuff
	// 	// Note(Leo): these seem like dot products
	// 	f32 a = pow2(v.x) + pow2(v.y);
	// 	f32 b = 2 * (p.x * v.x + p.y * v.y);
	// 	f32 c = pow2(p.x) + pow2(p.y);


	// 	// Undefined
	// 	if (math::absolute(a) < 0.00001f)
	// 	{
	// 		continue;
	// 	}

	// 	// We are looking solutions for r2, not 0
	// 	f32 cr2 = c - pow2(collider.radius);

	// 	// discriminant
	// 	f32 D = b * b - 4 * a * cr2;

	// 	// No roots no collisions
	// 	// Also one root, no collision
	// 	if (D < 0.00001f)
	// 	{
	// 		continue;
	// 	}

	// 	f32 sqrtD = math::square_root(D);
	// 	f32 ht0 = (-b - sqrtD) / (2 * a);
	// 	f32 ht1 = (-b + sqrtD) / (2 * a);

	// 	f32 ht = math::min(ht0, ht1);

	// 	if (ht > 0.00001f && ht < rayLength)
	// 	{
	// 		v3 pAtHtMin = p + v * ht;
	// 		if (pAtHtMin.z > -halfHeight && pAtHtMin.z < halfHeight)
	// 		{
	// 			hit = true;
	// 			if (rayHitSquareDistance > ht*ht)
	// 			{
	// 				rayHitSquareDistance = ht*ht;

	// 				outResult->hitPosition = pAtHtMin + center;

	// 				// Note(Leo): Hitposition - center, is weird
	// 				v3 hitNormal = p + v * ht;
	// 				hitNormal.z = 0;
	// 				hitNormal = normalize_v3(hitNormal);
	// 				outResult->hitNormal = hitNormal;
	// 			}
	// 		}
	// 	}
	// }

	return hit;
	// return false;
}