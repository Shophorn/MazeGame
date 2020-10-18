struct BoxCollider
{
	v3 			extents;
	quaternion 	orientation;
	v3 			center; 
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
	f32 halfHeight;
	v3 center;
};

struct PrecomputedBoxCollider
{
	m44 transform;
	m44 inverseTransform;
};

struct CollisionSystem3D
{
	Array<PrecomputedBoxCollider> 	staticBoxColliders;
	Array<CylinderCollider> 		staticCylinderColliders;

	Array<PrecomputedBoxCollider> 	submittedBoxColliders;
	Array<CylinderCollider> 		submittedCylinderColliders;



	HeightMap 	terrainCollider;
	v3 			terrainOffset;
};

internal CollisionSystem3D init_collision_system(MemoryArena & allocator)
{
	CollisionSystem3D system = {};

	system.staticBoxColliders = push_array<PrecomputedBoxCollider>(allocator, 100, ALLOC_GARBAGE);
	// system.staticCylinderColliders = push_array<CylinderCollider>(allocator, 100, ALLOC_GARBAGE);

	return system;
}

internal m44 compute_box_collider_transform(BoxCollider const & collider, Transform3D const & transform)
{
	m44 transformMatrix = transform_matrix(transform);
	m44 colliderMatrix 	= transform_matrix(collider.center, collider.orientation, collider.extents);

	return transformMatrix * colliderMatrix;
}

internal m44 compute_inverse_box_collider_transform(BoxCollider const & collider, Transform3D const & transform)
{
	m44 colliderMatrix = inverse_transform_matrix(collider.center, collider.orientation, collider.extents);
	m44 transformMatrix = inverse_transform_matrix(transform);

	return colliderMatrix * transformMatrix;
}

internal void clear_colliders(CollisionSystem3D & system)
{
	// Note(Leo): This is reseted, so that it is empty for the new frame
	system.submittedBoxColliders 		= push_array<PrecomputedBoxCollider>(*global_transientMemory, 100, ALLOC_GARBAGE);
	system.submittedCylinderColliders 	= push_array<CylinderCollider>(*global_transientMemory, 100, ALLOC_GARBAGE);
}

internal void submit_cylinder_collider(CollisionSystem3D & system, CylinderCollider collider, Transform3D const & transform)
{
	// Todo(Leo): we should assert these, but there are too many things, so I dont care today
	// Assert(transform.rotation.w == 1 && "We do not currently support rotated cylinder colliders");
	// Assert(abs_f32(transform.scale.x - transform.scale.y) < 0.00001f && "Scaling on xy plane must be uniform");

	collider.center 	+= transform.position;
	collider.radius 	*= transform.scale.x;
	collider.halfHeight *= transform.scale.z;
	
	system.submittedCylinderColliders.push(collider);
}

internal void submit_box_collider(CollisionSystem3D & system, BoxCollider collider, Transform3D const & transform)
{
	m44 transformMatrix = compute_box_collider_transform(collider, transform);
	m44 inverseMatrix 	= compute_inverse_box_collider_transform(collider, transform);
	system.submittedBoxColliders.push({transformMatrix, inverseMatrix});
}	

internal void push_static_box_collider(CollisionSystem3D & system, BoxCollider collider, Transform3D const & transform)
{
	m44 transformMatrix = compute_box_collider_transform(collider, transform);
	m44 inverseMatrix 	= compute_inverse_box_collider_transform(collider, transform);
	system.staticBoxColliders.push({transformMatrix, inverseMatrix});
}	

/// -------------- TERRAIN ---------------

internal f32 get_terrain_height(CollisionSystem3D const & system, v2 position)
{
	position.x -= system.terrainOffset.x;
	position.y -= system.terrainOffset.y;
	f32 value = get_height_at(&system.terrainCollider, position);
	return value;
}

/// ----------- COLLISION MATH ---------------

internal bool32 ray_box_collisions(	Array<PrecomputedBoxCollider> & colliders,
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
			memory_swap(xDistanceToMin, xDistanceToMax);
		}

		f32 yDistanceToMin = (min.y - colliderSpaceRayStart.y) * inverseDirection.y;
		f32 yDistanceToMax = (max.y - colliderSpaceRayStart.y) * inverseDirection.y;
		if (inverseDirection.y < 0)
		{
			memory_swap(yDistanceToMin, yDistanceToMax);
		}
		
		f32 zDistanceToMin = (min.z - colliderSpaceRayStart.z) * inverseDirection.z;
		f32 zDistanceToMax = (max.z - colliderSpaceRayStart.z) * inverseDirection.z;
		if (inverseDirection.z < 0)
		{
			memory_swap(zDistanceToMin, zDistanceToMax);
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
			f32 sqrDistance = square_v3_length(hitPosition - rayStart);

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
					outResult->hitNormal = normalize_v3(outResult->hitNormal);
				}

			}

			// f32 sqrDistance = square_v3_length(outResult->hitPosition, rayStart);
			// if (sqr)
			// rayHitSquareDistance = 
			// // return true;
		}
	}
	return hit;
}

internal bool32 ray_cylinder_collisions (Array<CylinderCollider> colliders, Ray ray, RaycastResult * outResult, f32 * rayHitSquareDistance)
{
	bool32 hit = false;

	for (auto const & collider : colliders)
	// for (s32 i = 0; i < count; ++i)
	{
		// auto & collider = colliders[i];

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
			f32 distanceFromCenter = v2_length(pAtVttMin.xy);
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

	hit = hit || ray_box_collisions(system->submittedBoxColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);
	hit = hit || ray_box_collisions(system->staticBoxColliders, rayStart, normalizedRayDirection, rayLength, outResult, &rayHitSquareDistance);

	hit = hit || ray_cylinder_collisions(	system->staticCylinderColliders,
											{rayStart, normalizedRayDirection, rayLength},
											outResult,
											&rayHitSquareDistance);

	hit = hit || ray_cylinder_collisions(	system->submittedCylinderColliders,
											{rayStart, normalizedRayDirection, rayLength},
											outResult,
											&rayHitSquareDistance);

	return hit;
}


internal void collisions_debug_draw_colliders(CollisionSystem3D const & system)
{
	for (auto const & collider : system.submittedBoxColliders)
	{
		debug_draw_box(collider.transform, colour_muted_green);
	}

	for (auto const & collider : system.staticBoxColliders)
	{
		debug_draw_box(collider.transform, colour_dark_green);
	}

	for (auto const & collider : system.staticCylinderColliders)
	{
		debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green);
		debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green);
	}

	for (auto const & collider : system.submittedCylinderColliders)
	{
		debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green);
		debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green);
	}
}
