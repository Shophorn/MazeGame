/*
Leo Tamminen
Trees version 3

TODO(Leo):
	interpolate beziers with even distances
*/

struct Tree3Node
{
	v3 			position;
	quaternion 	rotation;
	f32 		size; 			// Note(Leo): this may refer to diameter, but it does not have to
};

// Todo(Leo): Seems like we could benefit from array, but it was stupid to use, so must make some
// dumber, easier to use thing
struct Tree3Branch
{
	s32	 		nodeCapacity;
	s32 		nodeCount;
	Tree3Node * nodes;
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

void initialize_test_trees_3(MemoryArena & allocator, Trees3 & trees)
{
	trees.capacity 	= 1;
	trees.count 	= trees.capacity;
	trees.memory 	= push_memory<Tree3>(allocator, trees.capacity, 0);

	Tree3 & tree = trees.memory[0];

	tree.branchCapacity = 10;
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

	// tree.branchCount = 1;

}

template<typename T, s32 Size>
struct ConstArray
{
	T memory[Size];

	constexpr static s32 size = Size;

	T const & operator [] (s32 index) const
	{
		Assert(index < Size);
		return memory[index];
	}
};

void build_tree_3_mesh(Tree3 const & tree, DynamicMesh & mesh)
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

		// /// BOTTOMER, sphere below the lowest vertex loop
		// {
		// 	s32 bottomSphereLoops = 2; // Note(Leo): lets say this inlcudes first "actual" vertex loop

		// 	f32 angleStep = (0.5f * π) / bottomSphereLoops;

		// 	f32 radius = branch.nodes[0].size / 2;

		// 	mesh.vertices[0] = { .position = {0, 0, -radius}};
		// 	mesh.vertexCount = 1;

		// 	for (s32 loopIndex = 0; loopIndex < bottomSphereLoops; ++loopIndex)
		// 	{
		// 		f32 angle = (1 + loopIndex) * angleStep;

		// 		f32 cos = cosine(angle);	// xy, width
		// 		f32 sin = sine(angle);		// z, heigth

		// 		f32 z = sin * -radius;

		// 		for (s32 i = 0; i < verticesInLoop; ++i)
		// 		{
		// 			v2 xy = baseVertexPositions[i].xy * cos * radius;
		// 			v3 vertexPosition;
		// 			vertexPosition.xy = xy;
		// 			vertexPosition.z = z;
		// 			mesh.vertices[mesh.vertexCount + i] = {.position = vertexPosition};
		// 		}

		// 		if(loopIndex == 0)
		// 		{
		// 			for (s32 i = 0; i < verticesInLoop; ++i)
		// 			{
		// 				mesh.indices[mesh.indexCount++] = 0;
		// 				mesh.indices[mesh.indexCount++] = i;
		// 				mesh.indices[mesh.indexCount++] = (i + 1) % verticesInLoop;
		// 			}
		// 		}
		// 		else
		// 		{
		// 			for (s32 i = 0; i < verticesInLoop; ++i)
		// 			{
		// 				mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
		// 				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
		// 				mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount; 

		// 				mesh.indices[mesh.indexCount++] = ((i + 0) % verticesInLoop) + mesh.vertexCount; 
		// 				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop; 
		// 				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount; 
		// 			}
		// 		}


		// 		mesh.vertexCount += verticesInLoop;


		// 	}
		// }


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

#if 0
			f32 angle = fullAngle - 1 * angleStep;
			f32 sin = sine(angle);
			f32 cos = cosine(angle);

			for (s32 i = 0; i < verticesInLoop; ++i)
			{
				v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * size * cos;
				mesh.vertices[mesh.vertexCount + i] = {.position = pos + nodePosition + downDirection * sin * radius};
			}
			for(s32 i = 0; i < verticesInLoop; ++i)
			{
				mesh.indices[mesh.indexCount++] = bottomVertexIndex;
				mesh.indices[mesh.indexCount++] = mesh.vertexCount + ((i + 1) % verticesInLoop);
				mesh.indices[mesh.indexCount++] = mesh.vertexCount + i;
			}

			mesh.vertexCount += verticesInLoop;

			// -----------------------------------------------------------------------------

			angle = fullAngle - 2 * angleStep;
			sin = sine(angle);
			cos = cosine(angle);

			for (s32 i = 0; i < verticesInLoop; ++i)
			{
				v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * size * cos;
				mesh.vertices[mesh.vertexCount + i] = {.position = pos + nodePosition + downDirection * sin * radius};
			}
			
			for (s32 i = 0; i < verticesInLoop; ++i)
			{
				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;

				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount;
			}
			mesh.vertexCount += verticesInLoop;

			// -----------------------------------------------------------------------------

			angle = fullAngle - 3 * angleStep;
			sin = sine(angle);
			cos = cosine(angle);

			for (s32 i = 0; i < verticesInLoop; ++i)
			{
				v3 pos = rotate_v3(nodeRotation, baseVertexPositions[i]) * size * cos;
				mesh.vertices[mesh.vertexCount + i] = {.position = pos + nodePosition + downDirection * sin * radius};
			}
			
			for (s32 i = 0; i < verticesInLoop; ++i)
			{
				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;

				mesh.indices[mesh.indexCount++] = i + mesh.vertexCount;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount - verticesInLoop;
				mesh.indices[mesh.indexCount++] = ((i + 1) % verticesInLoop) + mesh.vertexCount;
			}
			mesh.vertexCount += verticesInLoop;

		#endif
			// s32 bottomSphereLoops = 1;
			// f32 angleStep = (0.5 * π) / bottomSphereLoops;

			// for (s32 loopIndex = 0; loopIndex < bottomSphereLoops; ++loopIndex)
			// {
			// 	f32 angle = (loopIndex + 1) * angleStep;
			// 	f32 cos = cosine(angle);
			// 	f32 sin = sine(angle);

			// 	f32 z = sin * -radius;

			// 	for (s32 i = 0; i < verticesInLoop; ++i)
			// 	{
			// 		v3 vertexPosition = position + baseVertexPositions[i] * cos * radius - v3 {0, 0, z};
			// 		mesh.vertices[i] = {.position = vertexPosition};
			// 	}

			// 	for(s32 i = 0; i < verticesInLoop; ++i)
			// 	{
			// 		mesh.indices[mesh.indexCount++] = bottomVertexIndex;
			// 		mesh.indices[mesh.indexCount++] = mesh.vertexCount + ((i + 1) % verticesInLoop);
			// 		mesh.indices[mesh.indexCount++] = mesh.vertexCount + i;
			// 	}
			// 	mesh.vertexCount += verticesInLoop;
	
			// }


		}

		for (s32 nodeIndex = 1; nodeIndex < branch.nodeCount; ++nodeIndex)
		{
			Tree3Node & previous 	= branch.nodes[nodeIndex - 1];
			Tree3Node & next 		= branch.nodes[nodeIndex];

			// Todo(Leo): expose in tree editor once we have that
			// Note(Leo): this can be tweaked for artistic purposes
			constexpr f32 tangentScale = 1.5;

			v3 bezier0 = previous.position;
			v3 bezier1 = previous.position + previous.size * tangentScale * rotate_v3(previous.rotation, up_v3);
			v3 bezier2 = next.position - next.size * tangentScale * rotate_v3(next.rotation, up_v3);
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

			// f32 a = previous.size;
			// f32 b = ;
			// f32 c = ;
			// f32 d = next.size;

			// auto bezier_lerp_f32 = [&](f32 t) ->f32
			// {
			// 	f32 ab = lerp_f32(a, b, t);
			// 	f32 bc = lerp_f32(b, c, t);
			// 	f32 cd = lerp_f32(c, d, t);

			// 	f32 abc = lerp_f32(ab, bc, t);
			// 	f32 bcd = lerp_f32(bc, cd, t);

			// 	f32 abcd = lerp_f32(abc, bcd, t);

			// 	return abcd;
			// };

			for (s32 loopIndex = 0; loopIndex < vertexLoopsInNodeSection; ++loopIndex)
			{
				f32 t = (loopIndex + 1) * loopTStep;

				v3 position 		= bezier_lerp_v3(t);
				f32 size 			= lerp_f32(previous.size, next.size, mathfun_smoother_f32(t));

				// Todo(Leo): maye bezier slerp this too, but let's not bother with that right now
				quaternion rotation = slerp_quaternion(previous.rotation, next.rotation, t);

				for (s32 i = 0; i < verticesInLoop; ++i)
				{
					v3 vertexPosition = rotate_v3(rotation, baseVertexPositions[i] * size) + position;
					mesh.vertices[mesh.vertexCount + i] = {.position = vertexPosition};
				}

				for (s32 i = 0; i <verticesInLoop; ++i)
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

		}
	}

	mesh_generate_normals(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);
	// mesh_generate_tangents(mesh.vertexCount, mesh.vertices, mesh.indexCount, mesh.indices);
}