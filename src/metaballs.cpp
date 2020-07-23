internal void add_cube(v3 position, f32 size, Vertex * vertices, s32 vertexOffset, u16 * indices, s32 indexOffset)
{

	/* 	__
	 __|__|__		
	|__|__|__|
	   |__|
	   |__|
	

	   0, 1,
	2, 3, 4, 5,
	6, 7, 8, 9	
	  10,11,
	  12,13,


	0 == 2 == 12
	1 == 5 == 13
	
	6 == 10
	9 == 11
	*/

	v3 positions [] =
	{
		{-0.5, 0.5, 0.5},	// 0
		{ 0.5, 0.5, 0.5}, // 1
		{-0.5, 0.5, 0.5}, // 2 == 0
		{-0.5,-0.5, 0.5}, // 3
		{ 0.5,-0.5, 0.5}, // 4
		{ 0.5, 0.5, 0.5}, // 5 == 1
		{-0.5, 0.5,-0.5}, // 6
		{-0.5,-0.5,-0.5}, // 7
		{ 0.5,-0.5,-0.5}, // 8
		{ 0.5, 0.5,-0.5}, // 9
		{-0.5, 0.5,-0.5}, // 10 == 6
		{ 0.5, 0.5,-0.5}, // 11 == 9
		{-0.5, 0.5, 0.5}, // 12 == 0
		{ 0.5, 0.5, 0.5}, // 13 == 0
	};

	v2 texcoords [] = 
	{
				{0,1}, {1,1},
		{-1,0}, {0,0}, {1,0}, {2,0},
		{-1,-1}, {0,-1}, {1,-1}, {2,-1},
				{0,-2}, {1,-2},
				{0,-3}, {1,-3},
	};

	// static_assert(array_count(positions) == array_count(texcoords));

	u16 indices_ [] =
	{
		// -X and X faces
		6, 7, 2, 2, 7, 3,
		6, 7, 2, 2, 7, 3,
		8, 9, 4, 4, 9, 5,

		// -Y and Y faces
		7, 8, 3, 3, 8, 4,
		11, 10, 13, 13, 10, 12,

		// -Z and Z faces
		8, 7, 11, 11, 7, 10,
		3, 4, 0, 0, 4, 1,
	};

	for(s32 i = 0; i < array_count(positions); ++i)
	{
		vertices[i + vertexOffset].position 	= (positions[i] * size) + position;
		vertices[i + vertexOffset].normal 		= normalize_v3(positions[i]);
		vertices[i + vertexOffset].texCoord 	= texcoords[i];
	}

	for (s32 i = 0; i < array_count(indices_); ++i)
	{
		indices[i + indexOffset] = indices_[i] + vertexOffset;
	}
}

internal MeshHandle generate_metaball()
{
	u8 field [] =
	{	
		1,0,0,0,1,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		1,0,0,0,1,

		0,1,1,1,0,
		1,0,0,0,1,
		1,0,0,0,1,
		1,0,0,0,1,
		0,1,1,1,0,

		0,1,0,1,0,
		1,0,0,0,1,
		1,0,1,0,0,
		1,0,0,0,1,
		0,1,0,1,0,

		0,1,1,1,0,
		1,0,0,0,1,
		1,0,0,0,1,
		1,0,0,0,1,
		0,1,1,1,0,

		1,0,0,0,1,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		1,0,0,0,1,
	};


	v3 fieldResolution = {5,5,5};
	auto sample_field = [&field, fieldResolution](s32 x, s32 y,  s32 z) -> u8
	{
		if (x < 0 || x >= fieldResolution.x || y < 0 || y >= fieldResolution.y || z < 0 || z >= fieldResolution.z)
		{
			return 0;
		}

		s32 index = x + y * fieldResolution.x + z * fieldResolution.x * fieldResolution.y;
		return field[index];
	};

	s32 vertexCountPerCube 	= 14;
	s32 indexCountPerCube 	= 36;
	s32 cubeCapacity 		= 100000;
	s32 vertexCapacity 		= vertexCountPerCube * cubeCapacity;
	s32 indexCapacity 		= indexCountPerCube * cubeCapacity;

	Vertex * vertices 	= push_memory<Vertex>(*global_transientMemory, vertexCapacity, ALLOC_NO_CLEAR);
	u16 * indices 		= push_memory<u16>(*global_transientMemory, indexCapacity, ALLOC_NO_CLEAR);

	u32 vertexCount = 0;
	u32 indexCount = 0;

	struct MarchingCubesCase
	{
		s32 vertexCount;
		v3 	vertices [10];
		s32 indexCount;
		u16 indices[15];
	};

	MarchingCubesCase cases [256] = {};
	f32 h = 0.5;

	/// SINGLE CORNER CASES
	cases[1] = {3, {{0,-h,-h}, {-h,0,-h},{-h,-h,0}}, 3,{0,1,2}};	
	cases[2] = {3, {{0,-h,-h}, {h,-h,0},{h,0,-h}}, 3,{0,1,2}};	
	cases[4] = {3, {{0,h,-h}, {-h,h,0}, {-h,0,-h}}, 3, {0,1,2}};
	cases[8] = {3, {{0,h,-h}, {h,0,-h}, {h,h,0}}, 3, {0,1,2}};

	cases [16] 	= {3, {{0,-h,h}, {-h,-h,0},{-h,0,h}}, 3,{0,1,2}};
	cases [32] 	= {3, {{0,-h,h}, {h,0,h}, {h,-h,0}}, 3,{0,1,2}};
	cases [64] 	= {3, {{0,h,h}, {-h,0,h}, {-h,h,0}}, 3,{0,1,2}};
	cases [128] = {3, {{0,h,h}, {h,h,0}, {h,0,h}}, 3,{0,1,2}};

	/// TWO ADJACENT CORNERS, X
	cases [3] 	= {4, { {h, 0, -h}, {-h, 0, -h}, {h, -h, 0}, {-h, -h, 0} },	6, {0,1,2, 2,1,3}};
	cases [12] 	= {4, { {-h, 0, -h}, {h, 0, -h}, {-h,h,0}, {h, h, 0} },		6, {0,1,2, 2,1,3}};
	cases [48] 	= {4, { {h, -h, 0}, {-h, -h, 0}, {h, 0, h}, {-h, 0, h} }, 	6, {0,1,2, 2,1,3}};
	cases [192] = {4, { {-h,h,0}, {h,h,0}, {-h, 0, h}, {h,0,h} }, 			6, {0,1,2, 2,1,3}};
	
	/// TWO ADJACENT CORNERS, Y
	cases [5] 	= {4, { {0,-h,-h}, {0, h,-h}, {-h,-h,0}, {-h,h,0} }, 	6, {0,1,2, 2,1,3}};
	cases [10]	= {4, { {0,-h,-h}, {0, h,-h}, {h,-h,0}, {h,h,0} }, 		6, {0,2,1, 1,2,3}};
	cases [80] 	= {4, { {0,-h, h}, {0, h, h}, {-h,-h,0}, {-h,h,0} }, 	6, {0,2,1, 1,2,3}};
	cases [160]	= {4, { {0,-h, h}, {0, h, h}, {h,-h,0}, {h,h,0} }, 		6, {0,1,2, 2,1,3}};

	/// TWO ADJACENT CORNERS, Z
	cases [17] 	= {4, {{0,-h,-h}, {-h,0,-h},{0,-h,h},{-h,0,h}}, 6, {0,1,2, 2,1,3}};
	cases [34] 	= {4, {{h,0,-h}, {0,-h,-h}, {h,0,h}, {0,-h,h}}, 6, {0,1,2, 2,1,3}};
	cases [68] 	= {4, {{-h,0,-h}, {0,h,-h}, {-h,0,h}, {0,h,h}}, 6, {0,1,2, 2,1,3}};
	cases [136] = {4, {{0,h,-h}, {h,0,-h}, {0,h,h}, {h,0,h}}, 6, {0,1,2, 2,1,3}};

	/// WEIRD CORNER THING
	cases [22] = {	9, { {h, 0, -h}, {0, h, -h}, {h, -h, 0}, {-h, h,0}, {0,-h, h}, {-h, 0, h}, /*separate triangle:*/{0,-h,-h}, {-h,-h,0},{-h,0,-h} },
					15, {0,1,2, 2,1,3, 2,3,4, 4,3,5, 6,7,8}};

	cases [97] = {	9, {{0,h,h}, {h,0,h}, {-h,h,0}, {h,-h,0}, {-h,0,-h}, {0,-h,-h}, /*separate triangle:*/ {0,-h,h}, {-h,0,h}, {-h,-h,0}},
					15, {0,1,2, 2,1,3, 2,3,4, 4,3,5, 6,7,8}};

	/// DIAGONAL PIPES, Z
	cases[102] = {	8, {{-h,0,-h}, {0,-h,-h}, {-h,0,h}, {0,-h,h}, /*other side quad:*/{h,0,-h}, {0,h,-h},{h,0,h},{0,h,h}},
				  	12, {0,1,2, 2,1,3, 4,5,6, 6,5,7 }};
	cases[153] = {	8, {{0,-h,-h}, {h,0,-h}, {0,-h,h}, {h,0,h}, /*othet side quad:*/{0,h,-h}, {-h,0,-h},{0,h,h}, {-h,0,h}},
				  	12, {0,1,2, 2,1,3, 4,5,6, 6,5,7 }};

	/// HALF, X
	cases [85] = {4, {{0,-h,-h}, {0, h, -h}, {0,-h, h}, {0, h,h}}, 6, {0,1,2, 2,1,3}};
	cases [170] = {4, {{0,h,-h}, {0,-h,-h}, {0,h,h}, {0,-h,h}}, 6,{0,1,2, 2,1,3}};

	/// WEIRD DIAGONAL TENT
	cases [40] = {6, {{h,0,-h}, {h,-h,0}, {0,h,-h}, {0,-h,h}, {h,h,0}, {h,0,h}}, 12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases [72] = {6, {{-h,h,0}, {0,h,-h}, {-h,0,h}, {h, 0, -h}, {0,h,h}, {h,h,0}}, 12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};

	cases [130] = {6, {{h,h,0}, {h,0,-h}, {0,h,h}, {0,-h,-h},{h,0,h}, {h,-h,0}}, 12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases [132] = {6, {{0,h,-h}, {h,h,0}, {-h,0,-h}, {h,0,h}, {-h,h,0}, {0,h,h}}, 12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};

	/// THREE ON SINGLE FACE, X
	cases [19] = {5, {{0,-h,h}, {h,-h,0}, {-h,0,h}, {h,0,-h}, {-h,0,-h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [35] = {5, {{-h,-h,0}, {0,-h,h}, {-h,0,-h}, {h,0,h}, {h,0,-h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [49] = {5, {{h,-h,0}, {0,-h,-h}, {h,0,h}, {-h,0,-h}, {-h,0,h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [50] = {5, {{0,-h,-h}, {-h,-h,0}, {h,0,-h}, {-h,0,h}, {h,0,h}}, 9, {0,1,2, 2,1,3, 2,3,4}};

	cases [76] 	= {5, {{h,h,0}, {0,h,h}, {h,0,-h}, {-h,0,h}, {-h,0,-h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [140] = {5, {{0,h,h}, {-h,h,0}, {h,0,h}, {-h,0,-h}, {h,0,-h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [196] = {5, {{0,h,-h}, {h,h,0}, {-h,0,-h}, {h,0,h}, {-h, 0, h}}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases [200] = {5, {{-h,h,0}, {0,h,-h}, {-h,0,h}, {h,0,-h}, {h, 0,h}}, 9, {0,1,2, 2,1,3, 2,3,4}};

	logDebug(0) << "sizeof(cases) = " << sizeof(cases);

	for (s32 z = -1; z < fieldResolution.z; ++z)
	{
		for (s32 y = -1; y < fieldResolution.y; ++y)
		{
			for (s32 x = -1; x < fieldResolution.z; ++x)
			{
				u8 caseId = 1 * 	sample_field(x, 	y, 		z);
				caseId += 2 * 	sample_field(x + 1, y, 		z);
				caseId += 4 * 	sample_field(x, 	y + 1, 	z);
				caseId += 8 * 	sample_field(x + 1, y + 1, 	z);
				caseId += 16 * 	sample_field(x, 	y, 		z + 1);
				caseId += 32 * 	sample_field(x + 1, y, 		z + 1);
				caseId += 64 * 	sample_field(x, 	y + 1, 	z + 1);
				caseId += 128 * 	sample_field(x + 1, y + 1, 	z + 1);

				for (s32 i = 0; i < cases[caseId].vertexCount; ++i)
				{
					vertices[vertexCount + i].position = v3{(f32)x, (f32)y, (f32)z} + cases[caseId].vertices[i];
					// Todo(Leo): not right obviously, but helps seeing generated shapes for now
					vertices[vertexCount + i].texCoord = cases[caseId].vertices[i].xy;
				}

				for (s32 i = 0; i < cases[caseId].indexCount; ++i)
				{
					indices[indexCount + i] = vertexCount + cases[caseId].indices[i];
				}

				vertexCount += cases[caseId].vertexCount;
				indexCount += cases[caseId].indexCount;

				caseId = 0;


				switch(caseId)
				{
					case 16: // xzy
						vertices[vertexCount + 0].position = {(f32)x, y - 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, y - 0.5f, (f32)z};
						vertices[vertexCount + 2].position = {x - 0.5f, (f32)y, z + 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					case 32: // xyz
						vertices[vertexCount + 0].position = {(f32)x, y - 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, (f32)y, z + 0.5f};
						vertices[vertexCount + 2].position = {x + 0.5f, y - 0.5f, (f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;
	
					case 64: // xzy
						vertices[vertexCount + 0].position = {(f32)x, y + 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, (f32)y, z + 0.5f};
						vertices[vertexCount + 2].position = {x - 0.5f, y + 0.5f, (f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					case 128: // xyz
						vertices[vertexCount + 0].position = {(f32)x, y + 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, y + 0.5f, (f32)z};
						vertices[vertexCount + 2].position = {x + 0.5f, (f32)y, z + 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					//////////////////////////////////////////////////////////////////
					/// INVERTED ONE CORNER CASES

					case 254: // 255 - 1
						vertices[vertexCount + 0].position = {(f32)x, y - 0.5f, z - 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, y - 0.5f, (f32)z};
						vertices[vertexCount + 2].position = {x - 0.5f, (f32)y, z - 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break; 

					case (255 - 2):
						vertices[vertexCount + 0].position = {(f32)x, 	y - 0.5f, 	z - 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, (f32)y, 	z - 0.5f};
						vertices[vertexCount + 2].position = {x + 0.5f, y - 0.5f, 	(f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;
						
						break;

					case (255 - 4):
						vertices[vertexCount + 0].position = {(f32)x, 	y + 0.5f, 	z - 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, (f32)y, 	z - 0.5f};
						vertices[vertexCount + 2].position = {x - 0.5f, y + 0.5f, 	(f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					case (255 - 8):
						vertices[vertexCount + 0].position = {(f32)x, 	y + 0.5f, 	z - 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, y + 0.5f, 	(f32)z};
						vertices[vertexCount + 2].position = {x + 0.5f, (f32)y, 	z - 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

					case (255 - 16): // xzy
						vertices[vertexCount + 0].position = {(f32)x, y - 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, (f32)y, z + 0.5f};
						vertices[vertexCount + 2].position = {x - 0.5f, y - 0.5f, (f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					case (255 - 32): // xyz
						vertices[vertexCount + 0].position = {(f32)x, y - 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, y - 0.5f, (f32)z};
						vertices[vertexCount + 2].position = {x + 0.5f, (f32)y, z + 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;
	
					case (255 - 64): // xzy
						vertices[vertexCount + 0].position = {(f32)x, y + 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x - 0.5f, y + 0.5f, (f32)z};
						vertices[vertexCount + 2].position = {x - 0.5f, (f32)y, z + 0.5f};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					case (255 - 128): // xyz
						vertices[vertexCount + 0].position = {(f32)x, y + 0.5f, z + 0.5f};
						vertices[vertexCount + 1].position = {x + 0.5f, (f32)y, z + 0.5f};
						vertices[vertexCount + 2].position = {x + 0.5f, y + 0.5f, (f32)z};

						indices[indexCount] = vertexCount;
						indices[indexCount + 1] = vertexCount + 1;
						indices[indexCount + 2] = vertexCount + 2;

						vertexCount += 3;
						indexCount += 3;

						break;

					//////////////////////////////////////////////////////////////////
					/// TWO CORNER CASES, X direction

					case 3:
					case 12:
					case 48:
					case 192:


					///////////////////////////////////////////////////////////////////
					/// TWO CORNER CASES, Y direction

					case 5:		// 1 + 4
					case 10: 	// 2 + 8
					case 80:	// 16 + 64
					case 160:	// 32 + 128
					{

					} break;

					default:
						break;
				}
			}
		}
	}


	f32 meshResolution = 22;

	f32 scale = 0.2;

	// for (s32 z = 0; z < meshResolution; ++z)
	// {
	// 	for (s32 y = 0; y < meshResolution; ++y)
	// 	{
	// 		for (s32 x = 0; x < meshResolution; ++x)
	// 		{
	// 			s32 fieldX = x / meshResolution * fieldResolution;
	// 			s32 fieldY = y / meshResolution * fieldResolution;
	// 			s32 fieldZ = z / meshResolution * fieldResolution;

	// 			s32 fieldIndex = fieldZ * fieldResolution * fieldResolution + fieldY * fieldResolution + fieldX;

	// 			if (field[fieldIndex] == 1)
	// 			{
	// 				Assert(vertexCount + vertexCountPerCube <= vertexCapacity);
	// 				Assert(indexCount + indexCountPerCube <= indexCapacity);

	// 				add_cube({x * scale, y * scale, z * scale}, scale, vertices, vertexCount, indices, indexCount);
	// 				vertexCount += vertexCountPerCube;
	// 				indexCount += indexCountPerCube;
	// 			}
	// 		}
	// 	}
	// }

	// add_cube({0, 0, 0}, 1, vertices, 0, indices, 0);



	// vertices[0].position = {0,0,0};
	// vertices[1].position = {1,0,0};
	// vertices[2].position = {0,1,0};
	
	// indices[0] = 0;//vertexCount;
	// indices[1] = 1;//vertexCount + 1;
	// indices[2] = 2;//vertexCount + 2;

	// vertexCount = vertexCountPerCube;
	// indexCount = 15;//indexCountPerCube;

	mesh_generate_normals(vertexCount, vertices, indexCount, indices);
	mesh_generate_tangents(vertexCount, vertices, indexCount, indices);

	MeshAsset asset = 
	{
		{vertices, vertexCount, vertexCount},
		{indices, indexCount, indexCount}
	};

	MeshHandle resultMesh = platformApi->push_mesh(platformGraphics, &asset);
	return resultMesh;
}