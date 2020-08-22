/*
Leo Tamminen
Trees version 3

TODO(Leo):
	interpolate beziers with even distances
*/

enum Tree3NodeType : s32
{
	TREE_NODE_ROOT,
	TREE_NODE_STEM,
	TREE_NODE_APEX,
};

struct Tree3Node
{
	v3 			position;
	quaternion 	rotation;
	f32 		size; 			// Note(Leo): this may refer to diameter, but it does not have to
	f32 		age;

	Tree3NodeType 	type;
};

// Todo(Leo): Seems like we could benefit from array, but it was stupid to use, so we must make some
// dumber, easier to use thing
struct Tree3Branch
{
	s32	 		nodeCapacity;
	s32 		nodeCount;
	Tree3Node * nodes;

	// s32 parentBranchIndex 			= -1;
	// f32 distanceAlongParentBranch;
};

struct Tree3
{
	s32 			branchCapacity;
	s32 			branchCount;
	Tree3Branch * 	branches;
};

struct Trees3
{
	s32 	capacity;
	s32 	count;
	Tree3 * memory;
};

struct DynamicMesh
{
	u64 vertexCapacity;
	u64 vertexCount;
	Vertex * vertices;

	u64 indexCapacity;
	u64 indexCount;
	u16 * indices;
};

internal void flush_dynamic_mesh(DynamicMesh & mesh)
{
	mesh.vertexCount = 0;
	mesh.indexCount = 0;
};

void initialize_test_trees_3(MemoryArena & allocator, Trees3 & trees)
{
	trees.capacity 	= 2;
	trees.count 	= trees.capacity;
	trees.memory 	= push_memory<Tree3>(allocator, trees.capacity, 0);

	Tree3 & tree = trees.memory[0];

	tree.branchCapacity = 30;
	tree.branchCount 	= 2;
	tree.branches 		= push_memory<Tree3Branch>(allocator, tree.branchCapacity, 0);

	for (s32 i = 0; i < tree.branchCapacity; ++i)
	{
		Tree3Branch & branch = tree.branches[i];

		branch.nodeCapacity = 10;
		branch.nodeCount 	= 0;
		branch.nodes 		= push_memory<Tree3Node>(allocator, branch.nodeCapacity, 0);
	}

	tree.branches[0].nodes[tree.branches[0].nodeCount++] = 
	{
		.position 	= {0,0,0},
		.rotation 	= identity_quaternion,
		.size 		= 1.5,
	};

	tree.branches[0].nodes[tree.branches[0].nodeCount++] = 
	{
		.position 	= {1,2,3},
		.rotation 	= axis_angle_quaternion(right_v3, -0.2 * π),
		.size 		= 1
	};

	tree.branches[0].nodes[tree.branches[0].nodeCount++] = 
	{
		.position 	= {1,1,6},
		.rotation 	= identity_quaternion,
		.size 		= 0.35
	};

	tree.branches[0].nodes[tree.branches[0].nodeCount++] = 
	{
		.position 	= {1,0.8,8},
		.rotation 	= identity_quaternion,
		.size 		= 0.1
	};

	tree.branches[0].nodes[tree.branches[0].nodeCount++] = 
	{
		.position 	= {1,0.7,11},
		.rotation 	= identity_quaternion,
		.size 		= 0.05
	};

	// 2nd branch
	tree.branches[1].nodeCount = 0;
	tree.branches[1].nodes[tree.branches[1].nodeCount++] =
	{
		.position 	= {0.5, 1.0, 1.5},
		.rotation 	= axis_angle_quaternion(forward_v3, 0.45f * π),
		.size 		= 1,
	};

	v3 branchOrigin 	= tree.branches[1].nodes[0].position;
	v3 branchDirection 	= rotate_v3(tree.branches[1].nodes[0].rotation, up_v3);

	tree.branches[1].nodes[tree.branches[1].nodeCount++] =
	{
		.position = branchOrigin + branchDirection * 2.5,
		.rotation = axis_angle_quaternion(forward_v3, 0.35f * π),
		.size = 0.5,
	};

	branchOrigin 	= tree.branches[1].nodes[1].position;
	branchDirection = rotate_v3(tree.branches[1].nodes[1].rotation, up_v3);

	tree.branches[1].nodes[tree.branches[1].nodeCount++] =
	{
		.position = branchOrigin + branchDirection * 2.5,
		.rotation = axis_angle_quaternion(forward_v3, 0.25f * π),
		.size = 0.3,
	};

	/// -----------------------------------------------------------

	Tree3 & tree2 			= trees.memory[1];
	tree2.branchCapacity 	= 10;
	tree2.branchCount 		= 1;
	tree2.branches 			= push_memory<Tree3Branch>(allocator, tree2.branchCapacity, ALLOC_CLEAR);

	for (s32 i = 0; i < tree2.branchCapacity; ++i)
	{
		tree2.branches[i].nodeCapacity 	= 6;
		tree2.branches[i].nodeCount 	= 0;
		tree2.branches[i].nodes 		= push_memory<Tree3Node>(allocator, 10, ALLOC_CLEAR);
	}

	tree2.branches[0].nodeCount = 2;
	tree2.branches[0].nodes[0] = 
	{
		.position 	= {0,0,0},
		.rotation 	= identity_quaternion,
		.size 		= 0.05,
		.age 		= 0.1,
		.type 		= TREE_NODE_ROOT,
	};

	tree2.branches[0].nodes[1] = 
	{
		.position 	= {0,0,0},
		.rotation 	= identity_quaternion,
		.size 		= 0.01,
		.age 		= 0,
		.type 		= TREE_NODE_APEX,
	};

}

internal void grow_tree_3(Tree3 & tree, f32 elapsedTime)
{
	elapsedTime *= 3;

	constexpr f32 widthGrowSpeed = 0.01;
	constexpr f32 lengthGrowSpeed = 0.1;
	constexpr f32 apexTerminalAge = 20;

	for (s32 branchIndex = 0; branchIndex < tree.branchCount; ++branchIndex)
	{
		Tree3Branch & branch = tree.branches[branchIndex];

		// Todo(Leo): to do this requires actual hierarchy of nodes/branches
		// if (branchIndex > 0 && random_choice_probability(0.001))
		// {
		// 	swap(tree.branches[branchIndex], tree.branches[tree.branchCount -1]);
		// 	tree.branchCount 	-= 1;
		// 	branchIndex 		-=1;

		// 	continue;
		// }

		for (s32 nodeIndex = 0; nodeIndex < branch.nodeCount; ++nodeIndex)
		{
			Tree3Node & node 	= branch.nodes[nodeIndex];
			node.age 			+= elapsedTime;
			node.rotation 		= slerp_quaternion(node.rotation, identity_quaternion, 0.01 * elapsedTime);

			if (node.type == TREE_NODE_ROOT || node.type == TREE_NODE_STEM)
			{
				node.size += widthGrowSpeed * elapsedTime;
			}

			else if (node.type == TREE_NODE_APEX)
			{
				v3 direction 	= rotate_v3(node.rotation, up_v3);
				node.position 	+= direction * lengthGrowSpeed * elapsedTime;

				if (node.age > apexTerminalAge)
				{
					node.type = TREE_NODE_STEM;

					if (branch.nodeCount < branch.nodeCapacity)
					{	
						v3 axis 			= normalize_v3(random_inside_unit_square() - v3{0.5, 0.5, 0});
						f32 angle 			= random_value() * 0.5;
						quaternion rotation = axis_angle_quaternion(axis, angle);

						branch.nodes[branch.nodeCount++] =
						{
							.position 	= node.position, 
							.rotation 	= node.rotation * rotation,
							.size 		= 0.01,
							.age 		= 0,
							.type 		= TREE_NODE_APEX,
						};

						if (tree.branchCount < tree.branchCapacity && random_choice_probability(0.5))
						{
							Tree3Branch & newBranch = tree.branches[tree.branchCount];
							tree.branchCount 		+= 1;

							quaternion rotation = axis_angle_quaternion(axis, -2 * angle);

							newBranch.nodeCount = 2;
							newBranch.nodes[0] =
							{
								.position 	= node.position, 
								.rotation 	= node.rotation * rotation,
								.size 		= 0.03,
								.age 		= 0.1,
								.type 		= TREE_NODE_STEM,
							};

							newBranch.nodes[1] = 
							{
								.position 	= node.position,
								.rotation 	= node.rotation * rotation,
								.size 		= 0.01,
								.age 		= 0,
								.type 		= TREE_NODE_APEX,
							};
						}
					}
				}
			}
		}
	}
}

void build_tree_3_mesh(Tree3 const & tree, Leaves & leaves, DynamicMesh & mesh)
{
	for (s32 branchIndex = 0; branchIndex < tree.branchCount; ++branchIndex)
	{
		Tree3Branch const & branch = tree.branches[branchIndex];

		f32 v = 0;

		/*
		DONE:
		number of vertices in a loop to variable,
			->later make it depend on size of node and somehow combine where it changes
		interpolate positions
		interpolate with bezier curves by fixed t-values

		TODO:
		platform graphics based dynamic mesh, so no need to copy each f
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

		/// BOTTOM, only vertices, triangles are added on later rounds
		{
			v3 nodePosition 		= branch.nodes[0].position;
			quaternion nodeRotation = branch.nodes[0].rotation;
			f32 size 				= branch.nodes[0].size;

			f32 radius = size * 0.5f;

			v3 downDirection = rotate_v3(nodeRotation, -up_v3);

			s32 bottomVertexIndex = mesh.vertexCount;
			mesh.vertices[mesh.vertexCount++] = { .position = nodePosition + downDirection * radius };

			s32 bottomSphereLoops 	= 2;
			f32 fullAngle 			= 0.5f * π;
			f32 angleStep 			= fullAngle / bottomSphereLoops;

			Assert((mesh.vertexCount + bottomSphereLoops * verticesInLoop) <= mesh.vertexCapacity);
			Assert((mesh.indexCount + bottomSphereLoops * verticesInLoop * 6) <= mesh.indexCapacity);


			for (s32 loopIndex = 0; loopIndex < bottomSphereLoops; ++loopIndex)
			{
				f32 angle 	= fullAngle - (loopIndex + 1) * angleStep;
				f32 sin 	= sine(angle);
				f32 cos 	= cosine(angle);

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * size * cos;
					mesh.vertices[mesh.vertexCount + i] = {.position = pos + nodePosition + downDirection * sin * radius};
				}

				if (loopIndex == 0)
				{
					for(s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indexCount++] = bottomVertexIndex;
						mesh.indices[mesh.indexCount++] = mesh.vertexCount + ((i + 1) % verticesInLoop);
						mesh.indices[mesh.indexCount++] = mesh.vertexCount + i;
					}
				}
				else
				{
					for (s32 i = 0; i < verticesInLoop; ++i)
					{
						mesh.indices[mesh.indexCount++] = i + mesh.vertexCount - verticesInLoop;
						mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
						mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;

						mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;
						mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
						mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount;
					}
				}

				mesh.vertexCount += verticesInLoop;
			}
		}

		for (s32 nodeIndex = 1; nodeIndex < branch.nodeCount; ++nodeIndex)
		{
			Tree3Node & previous 	= branch.nodes[nodeIndex - 1];
			Tree3Node & next 		= branch.nodes[nodeIndex];

			// Todo(Leo): expose in tree editor once we have that
			// Note(Leo): this can be tweaked for artistic purposes
			constexpr f32 tangentScale = 1.5;

			f32 maxTangentLength = magnitude_v3(previous.position - next.position) / 3;

			f32 previousTangentLength 	= min_f32(maxTangentLength, previous.size * tangentScale);
			f32 nextTangentLength 		= min_f32(maxTangentLength, next.size * tangentScale);

			v3 bezier0 = previous.position;
			v3 bezier1 = previous.position + previousTangentLength * rotate_v3(previous.rotation, up_v3);
			v3 bezier2 = next.position - nextTangentLength * rotate_v3(next.rotation, up_v3);
			v3 bezier3 = next.position;

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

			Assert((mesh.vertexCount + vertexLoopsInNodeSection * verticesInLoop) <= mesh.vertexCapacity);
			Assert((mesh.indexCount + vertexLoopsInNodeSection * verticesInLoop * 6) <= mesh.indexCapacity);

			for (s32 loopIndex = 0; loopIndex < vertexLoopsInNodeSection; ++loopIndex)
			{
				f32 t = (loopIndex + 1) * loopTStep;

				v3 position 		= bezier_lerp_v3(t);
				f32 size 			= lerp_f32(previous.size, next.size, mathfun_smoother_f32(t));

				// Todo(Leo): maybe bezier slerp this too, but let's not bother with that right now
				quaternion rotation = slerp_quaternion(previous.rotation, next.rotation, t);

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 vertexPosition = rotate_v3(rotation, baseVertexPositions[i] * size) + position;
					mesh.vertices[mesh.vertexCount + i] = {.position = vertexPosition};
				}

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
					mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
					mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount; 

					mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount; 
					mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
					mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount; 
				}

				mesh.vertexCount += verticesInLoop;
			}

			if (next.type == TREE_NODE_APEX)
			{
				s32 leafCount = 10;

				for (s32 i = 0; i < leafCount; ++i)
				{
					f32 t = (f32)(i + 1) / leafCount;

					v3 position = bezier_lerp_v3(t);

					s32 leafIndex = leaves.count++;

					leaves.localPositions[leafIndex] 	= position;
					leaves.localRotations[leafIndex] 	= identity_quaternion;
					leaves.localScales[leafIndex] 		= 1;
					leaves.swayAxes[leafIndex] 			= up_v3;
				}
			}

		}
	}

	mesh_generate_normals(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);
	// mesh_generate_tangents(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);
}