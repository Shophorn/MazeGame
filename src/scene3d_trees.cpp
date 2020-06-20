/*
Leo Tamminen

Trees and leaves.
*/

struct Leaves
{
	s32 	count;
	v3 		position;
	f32 	radius;
	f32 	leafScale;
	s32 	processCount;

	// Todo(Leo): maybe put these into a struct
	v3 * 			localPositions;
	f32 * 			rotations;
	f32 * 			directions;
	v3 * 			axes;
	f32 * 			scales;
	quaternion * 	baseRotations;
};

struct Trees
{
	static constexpr f32 growthRate 		= 0.1f;
	static constexpr f32 waterAcceptDistance 	= 0.4f;

	s32 capacity;
	s32 count;

	Transform3D * transforms;

	// Todo(Leo): maybe put these into a struct
	f32 * growths;
	f32 * waterinesss;

	// f32 vitalities

	Leaves * leaves;

	ModelHandle model;
};

internal Leaves make_leaves(MemoryArena & allocator, s32 count)
{
	Leaves leaves = {};

	leaves.count 		= count;
	// leaves.position = leavesPositions[leavesIndex];
	leaves.radius 		= 10;
	leaves.leafScale 	= 1;

	leaves.localPositions 	= push_memory<v3>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.rotations 		= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.directions 		= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.axes 			= push_memory<v3>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.scales 			= push_memory<f32>(allocator, leaves.count, ALLOC_NO_CLEAR);
	leaves.baseRotations 	= push_memory<quaternion>(allocator, leaves.count, ALLOC_NO_CLEAR);

	/* Note(Leo): Initialize leaves in order biggest to smallest, so we can easily LOD them
	by limiting number of processed leaves. We do not currently have any such system, but
	this costs zero, so we do it.

	This to my best understanding represent uniform distribution, but it can be made to
	use other distributions as well. */
	f32 biggestScale 	= 1.0f;
	f32 smallestScale 	= 0.5f;
	f32 scaleStep 		= (biggestScale - smallestScale) / leaves.count;
	f32 scale 			= biggestScale;

	Assert(scaleStep > 0 && "Scale step is too small to make any change");

	for (s32 i = 0; i < leaves.count; ++i)
	{
		f32 horizontalAngle	= RandomRange(0, 2 * pi);
		f32 verticalAngle 	= RandomRange(-pi / 2, pi / 2);

		// Todo(Add more weight towards outer limit)
		f32 distance 	= RandomRange(0.6, 1);

		f32 sineHorizontal = sine(horizontalAngle);
		f32 cosineHorizontal = cosine(horizontalAngle);

		f32 x = cosineHorizontal;
		f32 y = sineHorizontal;

		v3 localPosition =
		{
			x * cosine(verticalAngle) * distance,
			y * cosine(verticalAngle) * distance,
			sine(verticalAngle) * distance
		};

		leaves.localPositions[i] = localPosition;

		// Todo(Leo): update names: rotations is current rotation values, directions is just which direction we are
		// rotation, counter clockwise or normal clockwise
		leaves.rotations[i] = RandomRange(-1, 1);
		leaves.directions[i] = Sign(RandomValue());


		// Note(Leo): leaf geometry is defined so that it points along positive y axis, so we rotate accordingly with -pi/2
		quaternion awayFromCenterRotation = axis_angle_quaternion(up_v3, horizontalAngle - pi / 2);

		v3 localYAxis 				= multiply_direction(rotation_matrix(awayFromCenterRotation), {0,1,0});
		v3 localXAxis 				= cross_v3(localYAxis, up_v3);
		f32 localXRotation 			= -pi / 4 + RandomRange(-pi / 12, pi / 12);

		// quaternion 
		leaves.baseRotations[i] 	= awayFromCenterRotation * axis_angle_quaternion(localXAxis, localXRotation);
		leaves.axes[i] 				= multiply_direction(rotation_matrix(leaves.baseRotations[i]), {0,1,0});

		leaves.scales[i] 	= scale;
		scale 				-= scaleStep;
	}

	return leaves;
}

internal void draw_leaves(Trees & trees)
{
	for (s32 leavesIndex = 0; leavesIndex < trees.count; ++leavesIndex)
	{
		Leaves & leaves = trees.leaves[leavesIndex]; 
		s32 drawCount 	= leaves.processCount;

		m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < drawCount; ++i)
		{
			quaternion rotation = axis_angle_quaternion(leaves.axes[i], leaves.rotations[i]);
			rotation 			= leaves.baseRotations[i] * rotation;

			leafTransforms[i] 	= transform_matrix(	leaves.position + leaves.localPositions[i] * leaves.radius, 
													rotation,
													make_uniform_v3(leaves.scales[i] * leaves.leafScale));
		}

		platformApi->draw_meshes(platformGraphics, drawCount, leafTransforms, {-1}, {-1});
	}
}

internal void draw_leaves(Leaves & leaves)
{
	s32 drawCount = math::min(leaves.count, leaves.processCount);

	m44 * leafTransforms = push_memory<m44>(*global_transientMemory, drawCount, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < drawCount; ++i)
	{
		quaternion rotation = axis_angle_quaternion(leaves.axes[i], leaves.rotations[i]);
		rotation 			= leaves.baseRotations[i] * rotation;

		leafTransforms[i] 	= transform_matrix(	leaves.position + leaves.localPositions[i] * leaves.radius, 
												rotation,
												make_uniform_v3(leaves.scales[i] * leaves.leafScale));
	}

	platformApi->draw_meshes(platformGraphics, drawCount, leafTransforms, {-1}, {-1});
}


internal void update_leaves(Trees & trees, f32 elapsedTime)
{
	/// UPDATE LEAVES
	for (s32 i = 0; i < trees.count; ++i)
	{
		constexpr f32 zPosition = 2.6;
		constexpr f32 growthRadiusMultiplier = 1;
		constexpr f32 growthScaleMin = 0.2; 
		constexpr f32 growthScaleMax = 1; 

		constexpr f32 maxGrowth = 20;

		f32 growth 		= trees.growths[i];
		Leaves & leaves = trees.leaves[i];

		leaves.position 	= trees.transforms[i].position + v3{0, 0, growth * zPosition};
		leaves.radius 		= growth * growthRadiusMultiplier;
		leaves.leafScale 	= 0.05f + math::clamp(growth / 5, 0.0f, 1.0f) * 0.95f;

		constexpr s32 minLeavesNumber 	= 2;
		f32 leafCountGrowthAmount 		= math::clamp (growth / 5, 0.0f, 1.0f);
		leaves.processCount 			= minLeavesNumber + leafCountGrowthAmount * (leaves.count - minLeavesNumber);

		for (s32 leafIndex = 0; leafIndex < leaves.processCount; ++leafIndex)
		{
			leaves.rotations[leafIndex] += leaves.directions[leafIndex] * elapsedTime;
			constexpr f32 rotationRange = 0.5;
			// Todo(Leo): make pingpong like functionality and remove direction from Leaves
			if(leaves.rotations[leafIndex] < -rotationRange)
			{
				leaves.directions[leafIndex] = 1;
			}
			else if (leaves.rotations[leafIndex] > rotationRange)
			{
				leaves.directions[leafIndex] = -1;
			}
		}
	}
}

internal void update_leaves(Leaves & leaves, f32 elapsedTime)
{
	for (s32 leafIndex = 0; leafIndex < leaves.processCount; ++leafIndex)
	{
		leaves.rotations[leafIndex] += leaves.directions[leafIndex] * elapsedTime;
		constexpr f32 rotationRange = 0.5;
		// Todo(Leo): make pingpong like functionality and remove direction from Leaves
		if(leaves.rotations[leafIndex] < -rotationRange)
		{
			leaves.directions[leafIndex] = 1;
		}
		else if (leaves.rotations[leafIndex] > rotationRange)
		{
			leaves.directions[leafIndex] = -1;
		}
	}
}

// Note(Leo): wild idea to emulate implicit this-pointer in regular member functions
#define IMPORT_TREES(treesInstance) s32 & count = (treesInstance).count;

internal Trees allocate_trees(MemoryArena & allocator, s32 capacity)
{
	Trees trees 		= {};
	trees.count 		= 0;
	trees.capacity  	= capacity;
	trees.transforms 	= push_memory<Transform3D>(allocator, capacity, 0);
	trees.growths 		= push_memory<f32>(allocator, capacity, 0);
	trees.waterinesss 	= push_memory<f32>(allocator, capacity, 0);
	
	trees.leaves 		= push_memory<Leaves>(allocator, capacity, ALLOC_NO_CLEAR);
	for (s32 i = 0; i < capacity; ++i)
	{
		trees.leaves[i] = make_leaves(allocator, 4000);
	}

	return trees;
}

// Note(Leo): return tree index
internal s32 add_new_tree(Trees & trees, v3 position)
{
	// IMPORT_TREES(trees);

	Assert(trees.count <= trees.capacity);

	s32 index = trees.count;

	trees.transforms[index].position 	= position;
	trees.transforms[index].rotation 	= identity_quaternion;
	trees.transforms[index].scale 		= {0,0,0};

	// Note(Leo): we assume that tree was created with a drop of water
	trees.waterinesss[index] 	= 0.5f;
	trees.growths[index] 		= 0;
	// trees.leavesIndices[index] 	= 0;

	trees.count += 1;
	return index;
}

internal void grow_trees(Trees & trees, f32 maxGrowth, f32 elapsedTime)
{
	for (s32 i = 0; i < trees.count; ++i)
	{
		if (trees.waterinesss[i] > 0)
		{
			f32 growth 				= trees.growthRate * elapsedTime;
			trees.growths[i] 		= math::min(maxGrowth, trees.growths[i] + growth);
			trees.waterinesss[i] 	-= growth;
	
			trees.transforms[i].scale = make_uniform_v3(trees.growths[i]);
		}
	}

	for (s32 i = 0; i < trees.count; ++i)
	{
		debug_draw_circle_xy(	trees.transforms[i].position + v3{0,0,0.5f},
								trees.growths[i] * trees.waterAcceptDistance,
								colour_aqua_blue, DEBUG_NPC);
	}
}
