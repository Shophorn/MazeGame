template<typename T>
struct Array2
{
	u64 capacity;
	u64 count;
	T * memory;

	T & operator[](u64 index)
	{ 
		Assert(index < count);
		return memory[index];
	}

	T operator [](u64 index) const
	{
		Assert(index < count);
		return memory[index];
	}

	bool has_room_for(u64 count)
	{
		return (this->capacity - this->count - count) > 0;
	}
};

template<typename T>
internal Array2<T> push_array_2(MemoryArena & allocator, u64 capacity, AllocFlags flags)
{
	Array2<T> result = {capacity, 0, push_memory<T>(allocator, capacity, flags)};
	return result;
}

template<typename T>
internal void clear_array_2(Array2<T> & array)
{
	memset(array.memory, 0, sizeof(T) * array.capacity);
}

struct DynamicMesh
{
	Array2<Vertex> 	vertices;
	Array2<u16> 	indices;
};

internal DynamicMesh push_dynamic_mesh(MemoryArena & allocator, u64 vertexCapacity, u64 indexCapacity)
{
	DynamicMesh mesh =
	{
		.vertices 	= push_array_2<Vertex>(allocator, vertexCapacity, ALLOC_CLEAR),
		.indices 	= push_array_2<u16>(allocator, indexCapacity, ALLOC_CLEAR)
	};
	return mesh;
}

internal void flush_dynamic_mesh(DynamicMesh & mesh)
{
	mesh.vertices.count = 0;
	mesh.indices.count 	= 0;
};

/*
Leo Tamminen
Trees version 3

TODO(Leo):
	interpolate beziers with even distances

	SELF PRUNING:
	Compute incoming light for each branch section, and add them up from child to parent branch.
	Compute a threshold value based on branch section and its cumulative childrens' sizes.
	Any branch that does not meet the threshold is pruned

*/

enum Tree3NodeType : s32
{
	// TREE_NODE_ROOT,
	TREE_NODE_STEM,
	TREE_NODE_APEX,
};

// Note(Leo): this is like a branch node
struct Tree3Node
{
	v3 			position;
	quaternion 	rotation;
	f32 		size; 			// Note(Leo): this may refer to diameter, but it does not have to
	f32 		age;

	Tree3NodeType 	type;
};

struct Tree3BranchSection
{
	s32 startNodeIndex;
	s32 endNodeIndex;
	f32 startNodeDistance;
	
	s32 parentBranchSectionIndex;
	s32 childBranchCount;

	f32 		nextBudPosition;
	quaternion 	nextBudRotation;
	s32 		budIndex;
};

struct Tree3Bud
{
	bool32 		hasLeaf;
	s32 		leafIndex[2];
	f32 		age;

	f32 		distanceFromStartNode;
	s32 		parentBranchSectionIndex;

	// Updated with branch section stuff
	v3 			position;
	quaternion 	rotation;
	f32 		size;

};

struct Tree3
{
	Array2<Tree3Node> 			nodes;
	Array2<Tree3Bud> 			buds;
	Array2<Tree3BranchSection> 	branchSections;

	Leaves 			leaves;
	DynamicMesh 	mesh;

	bool32 			enabled;

	v3 position;

	s32 highLightBudIndex;

	bool32 breakOnUpdate;

	f32 maxHeight = 10;
	f32 growSpeedScale = 1;
	f32 tangentScale = 5;
	f32 areaGrowthSpeed = 0.05;
	f32 lengthGrowthSpeed = 0.3;
	f32 maxHeightToWidthRatio = 20;
	f32 budInterval = 0.5;
	f32 budIntervalRandomness = 0.1;
	f32 budTerminalDistanceFromApex = 1.0;
	f32 budAngle = 2.06;
	f32 budAngleRandomness = 0.1;

	static constexpr auto serializedProperties = make_property_list
	(	
		serialized_property("maxHeight", 					&Tree3::maxHeight),
		serialized_property("growSpeedScale", 				&Tree3::growSpeedScale),
		serialized_property("tangentScale",	 				&Tree3::tangentScale),
		serialized_property("areaGrowthSpeed", 				&Tree3::areaGrowthSpeed),
		serialized_property("lengthGrowthSpeed", 			&Tree3::lengthGrowthSpeed),
		serialized_property("maxHeightToWidthRatio", 		&Tree3::maxHeightToWidthRatio),
		serialized_property("budInterval", 					&Tree3::budInterval),
		serialized_property("budIntervalRandomness", 		&Tree3::budIntervalRandomness),
		serialized_property("budTerminalDistanceFromApex", 	&Tree3::budTerminalDistanceFromApex),
		serialized_property("budAngle", 					&Tree3::budAngle),
		serialized_property("budAngleRandomness", 			&Tree3::budAngleRandomness)
	);
};

internal void grow_tree_3(Tree3 & tree, f32 elapsedTime)
{
	elapsedTime *= tree.growSpeedScale;

	for (s32 i = 0; i < tree.branchSections.count; ++i)
	{
		Tree3BranchSection & branchSection 	= tree.branchSections[i];
		Tree3Node & startNode 				= tree.nodes[branchSection.startNodeIndex];
		Tree3Node & endNode 				= tree.nodes[branchSection.endNodeIndex];

		{
			f32 maxRadius = highest_f32;

			if (branchSection.parentBranchSectionIndex >= 0)
			{
				s32 parentBranchStartNodeIndex 	= tree.branchSections[branchSection.parentBranchSectionIndex].startNodeIndex;
				s32 parentBranchChildCount 		= tree.branchSections[branchSection.parentBranchSectionIndex].childBranchCount;
				f32 parentBranchStartNodeSize 	= tree.nodes[parentBranchStartNodeIndex].size;

				f32 parentArea 				= π * parentBranchStartNodeSize * parentBranchStartNodeSize;
				f32 areaDividedForChildren 	= parentArea / parentBranchChildCount;
				f32 maxRadiurPerChildren 	= square_root_f32(areaDividedForChildren / π);

				maxRadius = maxRadiurPerChildren;
			}

			if (startNode.size < maxRadius)
			{
				f32 growthAmount 	= tree.areaGrowthSpeed / π * elapsedTime;
				startNode.size 		= square_root_f32(startNode.size * startNode.size + growthAmount);
			}
		}

		{
			f32 distanceFromStart 	= magnitude_v3(endNode.position - startNode.position);
			f32 distanceFromRoot 	= distanceFromStart + branchSection.startNodeDistance;

			f32 hFactor 			= clamp_f32(1 - (distanceFromRoot / tree.maxHeight), 0, 1);
			f32 hwRatio 			= distanceFromStart / startNode.size;
			f32 hwFactor 			= clamp_f32(1 - (hwRatio / tree.maxHeightToWidthRatio), 0, 1);
			f32 growFactor  		= hFactor * hwFactor * tree.lengthGrowthSpeed * elapsedTime;

			v3 direction 			= rotate_v3(endNode.rotation, up_v3);
			endNode.position 		+= direction * growFactor;
			
			f32 ratioToNextBud 		= (branchSection.nextBudPosition - distanceFromStart) / tree.budInterval;

			if (ratioToNextBud < 0)
			{
				// Assert(tree.budCount < tree.budCapacity);
				Assert(tree.buds.has_room_for(1));

				f32 budIntervalRandom 			= 1 + random_range(-tree.budIntervalRandomness, tree.budIntervalRandomness);
				branchSection.nextBudPosition 	= distanceFromStart + tree.budInterval * budIntervalRandom;

				branchSection.budIndex = tree.buds.count++;

				tree.buds[branchSection.budIndex].hasLeaf 		= true;
				tree.buds[branchSection.budIndex].leafIndex[0] 	= tree.leaves.count++;
				tree.buds[branchSection.budIndex].leafIndex[1] 	= tree.leaves.count++;
				tree.buds[branchSection.budIndex].age 			= 0;

				v3 branchDirection 							= rotate_v3(endNode.rotation, up_v3);

				f32 axisRotationAngle 						= tree.budAngle + random_range(-tree.budAngleRandomness, tree.budAngleRandomness);
				branchSection.nextBudRotation 				= branchSection.nextBudRotation * axis_angle_quaternion(branchDirection, axisRotationAngle);
				tree.buds[branchSection.budIndex].rotation  = branchSection.nextBudRotation;

				tree.buds[branchSection.budIndex].distanceFromStartNode  	= distanceFromStart;
				tree.buds[branchSection.budIndex].parentBranchSectionIndex 	= i;
			}
			
			// Note(Leo): these are updated each frame, as the apex bud moves with the tip of the branch
			tree.buds[branchSection.budIndex].position 	= endNode.position;
			tree.buds[branchSection.budIndex].size 		= 1 - clamp_f32(ratioToNextBud, 0, 1);

		}
	}


	for (s32 i = 0; i < tree.buds.count; ++i)
	{
		Tree3Bud & bud = tree.buds[i];

		bud.age += elapsedTime;

		Tree3Node const & branchEndNode = tree.nodes[tree.branchSections[bud.parentBranchSectionIndex].endNodeIndex];
		f32 distanceFromApex 			= magnitude_v3(bud.position - branchEndNode.position);

		if (bud.hasLeaf && (distanceFromApex > tree.budTerminalDistanceFromApex))
		{
			bud.hasLeaf = false;

			tree.leaves.localPositions[bud.leafIndex[0]] 	= {};
			tree.leaves.localRotations[bud.leafIndex[0]] 	= {};
			tree.leaves.localScales[bud.leafIndex[0]] 		= 0;
			tree.leaves.swayAxes[bud.leafIndex[0]] 		= {};

			tree.leaves.localPositions[bud.leafIndex[1]] 	= {};
			tree.leaves.localRotations[bud.leafIndex[1]] 	= {};
			tree.leaves.localScales[bud.leafIndex[1]] 		= 0;
			tree.leaves.swayAxes[bud.leafIndex[1]] 		= {};

			v3 axis 			= rotate_v3(bud.rotation, forward_v3);
			quaternion rotation = bud.rotation * axis_angle_quaternion(axis, 0.25 * π);
			v3 direction 		= rotate_v3(rotation, up_v3);

			Assert(tree.nodes.has_room_for(2));
			s32 startNodeIndex 	= tree.nodes.count++;
			s32 endNodeIndex 	= tree.nodes.count++;

			constexpr f32 nodeStartSize = 0.01;

			tree.nodes[startNodeIndex] = 
			{
				.position = bud.position,
				.rotation = rotation,
				.size = nodeStartSize, 			// Note(Leo): this may refer to diameter, but it does not have to
				.age = 0, 
				.type = TREE_NODE_STEM
			};

			tree.nodes[endNodeIndex] = 
			{
				.position = bud.position + direction * nodeStartSize,
				.rotation = rotation,
				.size = nodeStartSize / 2,
				.age = 0,
				.type = TREE_NODE_APEX
			};

			s32 newBranchIndex = tree.branchSections.count++;
			Tree3BranchSection & newBranchSection = tree.branchSections[newBranchIndex];

			newBranchSection.startNodeIndex 			= startNodeIndex;
			newBranchSection.endNodeIndex 				= endNodeIndex;
			newBranchSection.startNodeDistance 			= bud.distanceFromStartNode;
			newBranchSection.parentBranchSectionIndex 	= bud.parentBranchSectionIndex;
			newBranchSection.childBranchCount 			= 0;
			newBranchSection.nextBudPosition 			= tree.budInterval;
			newBranchSection.nextBudRotation 			= rotation;
			newBranchSection.budIndex 					= tree.buds.count++;

			// Todo(Leo): reset new bud here
			Tree3Bud & newBud 	= tree.buds[newBranchSection.budIndex];
			newBud.hasLeaf	 	= true;
			newBud.leafIndex[0]	= tree.leaves.count++;
			newBud.leafIndex[1]	= tree.leaves.count++;
			newBud.age 			= 0;

			newBud.distanceFromStartNode 	= 0;
			newBud.parentBranchSectionIndex = newBranchIndex;

			// // Updated with branch section stuff
			// v3 			position;
			// quaternion 	rotation;
			// f32 		size;

			tree.branchSections[newBranchSection.parentBranchSectionIndex].childBranchCount += 1;
		}

		if (bud.hasLeaf)
		{
			tree.leaves.localPositions[bud.leafIndex[0]] 	= bud.position;
			tree.leaves.localRotations[bud.leafIndex[0]] 	= bud.rotation;
			tree.leaves.localScales[bud.leafIndex[0]] 		= bud.size;
			tree.leaves.swayAxes[bud.leafIndex[0]] 			= rotate_v3(bud.rotation, forward_v3);

			tree.leaves.localPositions[bud.leafIndex[1]] 	= bud.position;
			tree.leaves.localRotations[bud.leafIndex[1]] 	= bud.rotation * axis_angle_quaternion(rotate_v3(bud.rotation, up_v3), π);
			tree.leaves.localScales[bud.leafIndex[1]] 		= bud.size;
			tree.leaves.swayAxes[bud.leafIndex[1]] 			= rotate_v3(bud.rotation, forward_v3);
		}

		if (bud.hasLeaf)
		{
			FS_DEBUG_ALWAYS(debug_draw_circle_xy(bud.position + tree.position, 0.5, colour_bright_green));
		}
		else
		{
			FS_DEBUG_ALWAYS(debug_draw_circle_xy(bud.position + tree.position, 0.5, colour_bright_red));
		}
	}
}

void build_tree_3_mesh(Tree3 const & tree, Leaves & leaves, DynamicMesh & mesh)
{
	/*
	DONE:
	number of vertices in a loop to variable,
		->later make it depend on size of node and somehow combine where it changes
	interpolate positions
	interpolate with bezier curves by fixed t-values

	TODO:
	platform graphics based dynamic mesh, so no need to copy each frame
	*/


	constexpr s32 verticesInLoop = 6;

	local_persist v3 baseVertexPositions[verticesInLoop] = {};
	local_persist bool32 arraysInitialized = false;
	if (arraysInitialized == false)
	{
		arraysInitialized = true;

		f32 angleStep = 2 * π / verticesInLoop;

		for (s32 i = 0; i < verticesInLoop; ++i)
		{
			// Note(Leo): this describes radius, so "size" in tree nodes describe diameter
			baseVertexPositions[i].xy 	= rotate_v2({0.5f ,0}, i * angleStep);
		}
	}

	s32 vertexLoopsInNodeSection = 4;
	f32 loopTStep = 1.0f / vertexLoopsInNodeSection;

	/* DOCUMENT (Leo):
	1. generate first vertices outside of the loop
	2. generate other vertices interpolating between nodes
		always assuming(correctly) that there exists a vertex loop from previous node section
		
		Each section interpolates from t > 0 to t == 1.

	*/

	for (s32 branchIndex = 0; branchIndex < tree.branchSections.count; ++branchIndex)
	{
		Tree3Node const & startNode = tree.nodes[tree.branchSections[branchIndex].startNodeIndex];
		Tree3Node const & endNode = tree.nodes[tree.branchSections[branchIndex].endNodeIndex];

		/// BOTTOM DOME
		{
			v3 nodePosition 		= startNode.position;
			quaternion nodeRotation = startNode.rotation;
			f32 size 				= startNode.size;

			f32 radius = size * 0.5f;

			v3 downDirection = rotate_v3(nodeRotation, -up_v3);

			s32 bottomVertexIndex = mesh.vertices.count;
			mesh.vertices[mesh.vertices.count++] = { .position = nodePosition + downDirection * radius };

			s32 bottomSphereLoops 	= 2;
			f32 fullAngle 			= 0.5f * π;
			f32 angleStep 			= fullAngle / bottomSphereLoops;

			// Assert((mesh.vertices.count + bottomSphereLoops * verticesInLoop) <= mesh.vertexCapacity);
			// Assert((mesh.indexCount + bottomSphereLoops * verticesInLoop * 6) <= mesh.indexCapacity);

			Assert(mesh.vertices.has_room_for(bottomSphereLoops * verticesInLoop));
			Assert(mesh.indices.has_room_for(bottomSphereLoops * verticesInLoop * 6));

			for (s32 loopIndex = 0; loopIndex < bottomSphereLoops; ++loopIndex)
			{
				f32 angle 	= fullAngle - (loopIndex + 1) * angleStep;
				f32 sin 	= sine(angle);
				f32 cos 	= cosine(angle);

				s32 verticesStartCount = mesh.vertices.count;

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * size * cos;
					mesh.vertices[mesh.vertices.count++] = {.position = pos + nodePosition + downDirection * sin * radius};
				}

				if (loopIndex == 0)
				{
					for(s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indices.count++] = bottomVertexIndex;
						mesh.indices[mesh.indices.count++] = verticesStartCount + ((i + 1) % verticesInLoop);
						mesh.indices[mesh.indices.count++] = verticesStartCount + i;
					}
				}
				else
				{
					for (s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indices.count++] = i + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = i + verticesStartCount;

						mesh.indices[mesh.indices.count++] = i + verticesStartCount;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop;
						mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount;
					}
				}
			}
		}

		{
			// Todo(Leo): expose in tree editor once we have that
			// Note(Leo): this can be tweaked for artistic purposes
			f32 tangentScale 			= tree.tangentScale;

			f32 maxTangentLength 		= magnitude_v3(startNode.position - endNode.position) / 3;
			f32 previousTangentLength 	= min_f32(maxTangentLength, startNode.size * tangentScale);
			f32 nextTangentLength 		= min_f32(maxTangentLength, endNode.size * tangentScale);

			v3 bezier0 = startNode.position;
			v3 bezier1 = startNode.position + previousTangentLength * rotate_v3(startNode.rotation, up_v3);
			v3 bezier2 = endNode.position - nextTangentLength * rotate_v3(endNode.rotation, up_v3);
			v3 bezier3 = endNode.position;

			auto bezier_lerp_v3 = [&](f32 t) -> v3
			{
				v3 b01 = lerp_v3(bezier0, bezier1, t);
				v3 b12 = lerp_v3(bezier1, bezier2, t);
				v3 b23 = lerp_v3(bezier2, bezier3, t);

				v3 b012 = lerp_v3(b01, b12, t);
				v3 b123 = lerp_v3(b12, b23, t);

				v3 b0123 = lerp_v3(b012, b123, t);

				return b0123;
			};

			Assert(mesh.vertices.has_room_for(vertexLoopsInNodeSection * verticesInLoop));
			Assert(mesh.indices.has_room_for(vertexLoopsInNodeSection * verticesInLoop * 6));

			for (s32 loopIndex = 0; loopIndex < vertexLoopsInNodeSection; ++loopIndex)
			{
				f32 t 				= (loopIndex + 1) * loopTStep;
				v3 position 		= bezier_lerp_v3(t);
				f32 size 			= lerp_f32(startNode.size, endNode.size, t);

				// Todo(Leo): maybe bezier slerp this too, but let's not bother with that right now
				quaternion rotation = slerp_quaternion(startNode.rotation, endNode.rotation, t);

				s32 verticesStartCount = mesh.vertices.count;

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 vertexPosition = rotate_v3(rotation, baseVertexPositions[i] * size) + position;
					mesh.vertices[mesh.vertices.count++] = {.position = vertexPosition};
				}

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount; 

					mesh.indices[mesh.indices.count++] = ((i + 0) % verticesInLoop) + verticesStartCount; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount - verticesInLoop; 
					mesh.indices[mesh.indices.count++] = ((i + 1) % verticesInLoop) + verticesStartCount; 
				}

				// mesh.vertices.count += verticesInLoop;
			}
		}
	}

	mesh_generate_normals(mesh.vertices.count, mesh.vertices.memory, mesh.indices.count, mesh.indices.memory);
	// mesh_generate_tangents(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);
}

internal void reset_test_tree3(Tree3 & tree)
{
	clear_array_2(tree.nodes);
	clear_array_2(tree.buds);
	clear_array_2(tree.branchSections);

	tree.nodes.count = 2;
	tree.nodes[0] = 
	{
		.position 	= {0,0,0},
		.rotation 	= identity_quaternion,
		.size 		= 0.05,
		.age 		= 0.1,
		.type 		= TREE_NODE_STEM,
	};

	tree.nodes[1] = 
	{
		.position 	= {0,0,0},
		.rotation 	= identity_quaternion,
		.size 		= 0.01,
		.age 		= 0,
		.type 		= TREE_NODE_APEX,
	};

	tree.branchSections.count = 1;
	tree.branchSections[0].startNodeIndex = 0;
	tree.branchSections[0].endNodeIndex = 1;
	tree.branchSections[0].childBranchCount 		= 0;
	tree.branchSections[0].parentBranchSectionIndex = -1;
	tree.branchSections[0].nextBudPosition 			= tree.budInterval;
	tree.branchSections[0].nextBudRotation 			= identity_quaternion;
	tree.branchSections[0].budIndex 				= 0;

	flush_leaves(tree.leaves);
	flush_dynamic_mesh(tree.mesh);

	// tree.buds.count			= 1;
	// tree.leaves.count 		= 2;

	Tree3Bud & bud = tree.buds[tree.buds.count++];

	tree.buds[0].hasLeaf 					= true;
	tree.buds[0].leafIndex[0] 				= tree.leaves.count++;
	tree.buds[0].leafIndex[1] 				= tree.leaves.count++;
	tree.buds[0].age 						= 0;
	tree.buds[0].distanceFromStartNode 		= 0;
	tree.buds[0].parentBranchSectionIndex 	= 0;
	tree.buds[0].position 					= {};
	tree.buds[0].rotation 					= identity_quaternion;
	tree.buds[0].size 						= 0;

	build_tree_3_mesh(tree, tree.leaves, tree.mesh);
}

internal void initialize_test_tree_3(MemoryArena & allocator, Tree3 & tree, v3 position)
{
	tree.nodes 			= push_array_2<Tree3Node>(allocator, 1000, ALLOC_CLEAR);
	tree.buds 			= push_array_2<Tree3Bud>(allocator, 1000, ALLOC_CLEAR);
	tree.branchSections = push_array_2<Tree3BranchSection>(allocator, 1000, ALLOC_CLEAR);

	tree.leaves = make_leaves(allocator, 4000);
	tree.mesh 	= push_dynamic_mesh(allocator, 5000, 10000);

	reset_test_tree3(tree);

	tree.position = position;
}

internal void tree_gui(Tree3 & tree)
{
	gui_toggle("Enable Updates", &tree.enabled);
	gui_float_field("Grow Speed Scale", &tree.growSpeedScale);

	if (gui_button("Reset tree"))
	{
		reset_test_tree3(tree);
	}

	gui_line();

	gui_text("PROPERTIES");

	gui_float_field("Area Grow Speed", &tree.areaGrowthSpeed, {.min = 0});
	gui_float_field("Length Grow Speed", &tree.lengthGrowthSpeed, {.min = 0});
	gui_float_field("Tangent Scale", &tree.tangentScale, {.min = 0});
	gui_float_field("Max Height", &tree.maxHeight, {.min = 0});
	gui_float_field("Bud Interval", &tree.budInterval, {.min = 0});
	gui_float_slider("Bud Interval Randomness", &tree.budIntervalRandomness, 0, 1);
	gui_float_field("Max Height To Width Ratio", &tree.maxHeightToWidthRatio, {.min = 0});
	gui_float_field("Bud Terminal Distance From Apex", &tree.budTerminalDistanceFromApex, {.min = 0});

	gui_float_field("Bud Angle", &tree.budAngle, {.min = 0, .max = 2*π});
	gui_float_slider("Bud Angle Randomness", &tree.budAngleRandomness, 0, 1);

	gui_line();

	f32 budCount = tree.buds.count;
	gui_float_field("Bud Count", &budCount);

	gui_line();

	gui_toggle("Break On Update", &tree.breakOnUpdate);
}

internal void update_tree_3(Tree3 & tree, f32 elapsedTime)
{
	if (tree.breakOnUpdate)
	{
		tree.breakOnUpdate = false;
	}


	if (tree.enabled)
	{
		grow_tree_3(tree, elapsedTime);

		// flush_leaves(tree.leaves);
		flush_dynamic_mesh(tree.mesh);

		build_tree_3_mesh(tree, tree.leaves, tree.mesh);
	}


	// for(s32 b = 0; b < tree.branchCount; ++b)
	// {
	// 	// Tree3Branch & branch = tree.branches[b];
	// 	for (s32 n = 0; n < tree.nodeCount; ++n)
	// 	{
	// 		v3 position = branch.nodes[n].position + tree.position;
	// 		debug_draw_circle_xy(position, 0.2f, colour_bright_green, DEBUG_LEVEL_ALWAYS);

	// 		logDebug(0) << b << ";" << n << ": " << position;
	// 	}
	// // }
}