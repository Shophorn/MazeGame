u8 field [] =
{	
	1,0,0,0,1,0,0,
	0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,
	1,1,0,0,1,0,0,

	0,1,1,1,0,0,0,
	1,0,0,0,1,1,1,
	1,0,0,0,1,0,1,
	1,0,1,0,1,1,1,
	1,1,1,1,0,0,0,

	0,1,0,1,0,0,0,
	1,0,0,0,1,0,1,
	1,0,0,0,0,0,0,
	1,0,0,0,1,0,1,
	1,1,0,1,0,0,0,

	0,1,1,1,0,0,0,
	1,0,0,0,1,0,1,
	1,0,0,0,1,0,0,
	1,0,0,0,1,0,1,
	1,1,1,1,0,0,0,

	1,0,0,0,1,0,0,
	0,0,0,0,0,0,1,
	0,0,0,0,0,0,1,
	0,0,0,0,0,0,1,
	1,0,0,0,1,0,0,

	1,0,0,0,1,0,0,
	0,0,0,0,0,0,1,
	0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,
	1,0,0,0,1,0,0,

	1,1,1,1,1,0,0,
	1,1,1,1,1,1,1,
	1,1,1,1,1,0,1,
	1,1,1,1,1,1,1,
	1,1,1,1,1,0,0,

};

v3 fieldSize = {7,5,7};

internal bool32 point_inside_bounds(v3 point, v3 bounds)
{
	bool32 result = point.x > 0 || point.x < bounds.x 
					|| point.y > 0 || point.y < bounds.y
					|| point.z > 0 || point.z < bounds.z;
	return result;
}

// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp_f32(0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f);
    return interpolate(d2, d1, h) - k*h*(1.0f-h);
}

struct VoxelField
{
	f32 xSize;
	f32 ySize;
	f32 zSize;
	f32 * memory;
};

internal f32 f32_lerp_square(f32 const * values, v2 t)
{
	f32 v0 		= f32_lerp(values[0], values[1], t.x);
	f32 v1 		= f32_lerp(values[2], values[3], t.x);
	f32 result 	= f32_lerp(v0, v1, t.y);

	return result;
}

// struct SparseVoxel
// {
// 	bool32 	hasChildren;
// 	f32 	value;
// 	s32 	firstChild;	
// };

// struct SparseVoxelField
// {
// 	f32 xScale;
// 	f32 yScale;
// 	f32 zScale;

// 	s32 xCount;
// 	s32 yCount;
// 	s32 zCount;

// 	s32 			capacity;
// 	s32 			count;
// 	SparseVoxel * 	memory;
// };

// SparseVoxel testSparseVoxelField [] =
// {

// };

// internal f32 f32_lerp_cube(f32 (&values)[8], v3 t)
// {
// 	f32 v00 = f32_lerp(values[0], values[1], t.x);
// 	f32 v10 = f32_lerp(values[2], values[3], t.x);
// 	f32 v01 = f32_lerp(values[4], values[5], t.x);
// 	f32 v11 = f32_lerp(values[6], values[7], t.x);

// 	f32 v0 = f32_lerp(v00, v10, t.y);
// 	f32 v1 = f32_lerp(v01, v11, t.y);

// 	f32 v = f32_lerp(v0, v1, t.z);	

// 	return v;
// }

internal f32 sample_heightmap_for_mc(v3 position, void const * dataPtr)
{
	position.xy = position.xy * 100;

	HeightMap const * map = (HeightMap const *)dataPtr;

	s32 ix0 = floor_f32(position.x);
	s32 ix1 = ix0 + 1;
	f32 tx 	= ix0 - position.x;

	s32 iy0 = floor_f32(position.y);
	s32 iy1 = iy0 + 1;
	f32 ty 	= iy0 - position.y;

	ix0 = clamp_s32(ix0, 0, map->gridSize);
	iy0 = clamp_s32(iy0, 0, map->gridSize);

	ix1 = clamp_s32(ix1, 0, map->gridSize);
	iy1 = clamp_s32(iy1, 0, map->gridSize);

	f32 values [4] =
	{
		map->values[ix0 + iy0 * map->gridSize],
		map->values[ix1 + iy0 * map->gridSize],
		map->values[ix0 + iy1 * map->gridSize],
		map->values[ix1 + iy1 * map->gridSize],
	};
	f32 mapHeight 	= f32_lerp_square(values, {tx, ty});
	mapHeight *= 5;
	f32 height 		= position.z;

	return height - mapHeight;
}

internal f32 sample_voxel_field(v3 position, void const * dataPtr)
{
	VoxelField const * field = (VoxelField const *)dataPtr;


	// if (point_inside_bounds(position, fieldSize) == false)
	// 	return 10;

	s32 ix0 = floor_f32(position.x);
	s32 ix1 = ix0 + 1;
	f32 tx 	= ix0 - position.x;

	s32 iy0 = floor_f32(position.y);
	s32 iy1 = iy0 + 1;
	f32 ty 	= iy0 - position.y;

	s32 iz0 = floor_f32(position.z);
	s32 iz1 = iz0 + 1;
	f32 tz 	= iz0 - position.z;

	ix0 = clamp_s32(ix0, 0, field->xSize);
	iy0 = clamp_s32(iy0, 0, field->ySize);
	iz0 = clamp_s32(iz0, 0, field->zSize);

	ix1 = clamp_s32(ix1, 0, field->xSize);
	iy1 = clamp_s32(iy1, 0, field->ySize);
	iz1 = clamp_s32(iz1, 0, field->zSize);

	f32 v000 = field->memory[ix0 + iy0 * (s32)field->xSize + (s32)(iz0 * field->xSize * field->ySize)];
	f32 v100 = field->memory[ix1 + iy0 * (s32)field->xSize + (s32)(iz0 * field->xSize * field->ySize)];
	f32 v010 = field->memory[ix0 + iy1 * (s32)field->xSize + (s32)(iz0 * field->xSize * field->ySize)];
	f32 v110 = field->memory[ix1 + iy1 * (s32)field->xSize + (s32)(iz0 * field->xSize * field->ySize)];
	f32 v001 = field->memory[ix0 + iy0 * (s32)field->xSize + (s32)(iz1 * field->xSize * field->ySize)];
	f32 v101 = field->memory[ix1 + iy0 * (s32)field->xSize + (s32)(iz1 * field->xSize * field->ySize)];
	f32 v011 = field->memory[ix0 + iy1 * (s32)field->xSize + (s32)(iz1 * field->xSize * field->ySize)];
	f32 v111 = field->memory[ix1 + iy1 * (s32)field->xSize + (s32)(iz1 * field->xSize * field->ySize)];

	f32 v00 = f32_lerp(v000, v100, tx);
	f32 v10 = f32_lerp(v010, v110, tx);
	f32 v01 = f32_lerp(v001, v101, tx);
	f32 v11 = f32_lerp(v011, v111, tx);

	f32 v0 = f32_lerp(v00, v10, ty);
	f32 v1 = f32_lerp(v01, v11, ty);

	f32 v = f32_lerp(v0, v1, tz);

	return v;

	// s32 x = clamp_s32(floor_f32(position.x), 0, fieldSize.x);
	// s32 y = clamp_s32(floor_f32(position.y), 0, fieldSize.y);
	// s32 z = clamp_s32(floor_f32(position.z), 0, fieldSize.z);


	// return field[x + (s32)(y * fieldSize.x) + (s32)(z * fieldSize.x * fieldSize.y)];

}

internal f32 sample_test_voxel_field(v3 pos, void const * dataPtr)
{
	// u8 field [] =
	// {
	// 	0,0,0,
	// 	0,0,0,
	// 	0,0,0,

	// 	0,0,0,
	// 	0,1,0,
	// 	0,0,0,

	// 	0,0,0,
	// 	0,0,0,
	// 	0,0,0,
	// };

	// v3 fieldSize = {3,3,3};

	// u8 field [] = 
	// {
	// 	0,0,
	// 	1,0,

	// 	0,1,
	// 	1,1,
	// };

	// v3 fieldSize = {2,2,2};

	if (pos.x < 0 || pos.x > fieldSize.x || pos.y < 0 || pos.y > fieldSize.y || pos.z < 0 || pos.z > fieldSize.z)
	{
		return 1;
	}

	s32 x = floor_f32(pos.x);
	s32 y = floor_f32(pos.y) * fieldSize.x;
	s32 z = floor_f32(pos.z) * fieldSize.x * fieldSize.y;

	// logDebug(0) << pos << ", " << x << ", " << y << ", " << z;

	u8 value = field[x + y + z];

	return value == 1 ? -1 : 1;
};

internal f32 sample_four_sdf_balls (v3 pos, void const * data)
{
	v4 const * positions = (v4 const *)data;

	f32 d1 = magnitude_v3(pos - positions[0].xyz) - positions[0].w;
	f32 d2 = magnitude_v3(pos - positions[1].xyz) - positions[1].w;
	f32 d3 = magnitude_v3(pos - positions[2].xyz) - positions[2].w;
	f32 d4 = magnitude_v3(pos - positions[3].xyz) - positions[3].w;

	f32 result = opSmoothUnion(d1, d2, 0.5f);
	result = opSmoothUnion(result, d3, 0.5f);
	result = opSmoothUnion(result, d4, 0.5f);

	return result;
};

using MarchingCubesFieldSampleFunction = f32(*)(v3 position, void const * data);

internal void generate_mesh_marching_cubes(	s32 vertexCapacity, Vertex * vertices, u32 * outVertexCount,
											s32 indexCapacity, u16 * indices, u32 * outIndexCount,
											MarchingCubesFieldSampleFunction sample_field,
											void * fieldData,
											f32 gridScale)
{
	u32 vertexCount = 0;
	u32 indexCount = 0;

	PlatformTimePoint startTime = platformApi->current_time();

	constexpr s32 edges [24][2] = 
	{
		{0,1}, {0,2}, {1,3}, {2,3}, {0,4}, {1,5}, {2,6}, {3,7}, {4,5}, {4,6}, {5,7}, {6,7},
		{1,0}, {2,0}, {3,1}, {3,2}, {4,0}, {5,1}, {6,2}, {7,3}, {5,4}, {6,4}, {7,5}, {7,6},
	};

	struct BetterEdgeStruct
	{
		s32 vertexCount;
		s32 edgeIdIds[12];
		s32 triangleIndexCount;
		s32 indices[12];
	};

	constexpr BetterEdgeStruct betterCases [256] = 
	{
		{},
		{3, {0,1,4},				3, {0,1,2}},
		{3, {12, 5, 2,}, 			3, {0,1,2}},
		{4, {2, 1, 5, 4},			6, {0,1,2, 2,1,3}},
		{3, {13, 3, 6},				3, {0,1,2}},
		{4, {0, 3, 4, 6},			6, {0,1,2, 2,1,3}},
		{6, {12, 5, 2, 1, 3, 6},	6, {0,1,2, 3,4,5}},
		{5, {2, 15, 5, 6, 4},		9, {0,1,2, 2,1,3, 2,3,4}},
		{3, {14, 7, 15},			3, {0,1,2}},
		{6, {0, 1, 4, 14, 7, 15},	6, {0,1,2, 3,4,5}},
		{4, {15, 12, 7, 5},			6, {0,1,2, 2,1,3}},
		{5, {15, 1, 7, 4, 5},		9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {1, 2, 6, 7},			6, {0,1,2, 2,1,3}},
		{5, {0, 2, 4, 7, 6},		9, {0,1,2, 2,1,3, 2,3,4}},
		{5, {13, 12, 6, 5, 7},		9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {4, 5, 6, 7},			6, {0,1,2, 2,1,3}},

		// 16
		{3, {16, 9, 8}, 					3, {0,1,2}},
		{4, {0, 1, 8, 9}, 					6, {0,1,2, 2,1,3}},
		{6, {16, 9, 8, 12, 5, 2}, 			6, {0,1,2, 3,4,5}},
		{5, {8, 5, 9, 2, 1},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {13, 3, 6, 16, 9, 8},			6, {0,1,2, 3,4,5}},
		{5, {6, 9, 3, 8, 0},				9, {0,1,2, 2,1,3, 2,3,4}},
		{9, {12, 5, 2, 1, 3, 6, 16, 9, 8}, 	9, {0,1,2, 3,4,5, 6,7,8}},
		{6, {2, 3, 5, 6, 8, 9},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{6, {14, 7, 15, 16, 9, 8},			6, {0,1,2, 3,4,5}},
		{7, {0, 1, 8, 9, 14, 7, 15},		9, {0,1,2, 2,1,3, 4,5,6}},
		{7, {15, 12, 7, 5, 16, 9, 8},		9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {1, 9, 8, 5, 7, 15},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {1, 2, 6, 7, 16, 9, 7},			9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {7, 6, 9, 8, 0, 14},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{8, {13, 12,6, 5, 7, 16, 9, 8},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{5, {9, 8, 6, 5, 7}, 				9, {0,1,2, 2,1,3, 2,3,4}},

		// 32
		{3, {17, 8, 10},					3, {0,1,2}},
		{6, {0, 1, 4, 17, 20, 10}, 			6, {0,1,2, 3,4,5}},
		{4, {2, 12, 10, 20}, 				6, {0,1,2, 2,1,3}},
		{5, {4, 20, 1, 10, 2},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {13, 3, 6, 17, 20, 10},			6, {0,1,2, 3,4,5}},
		{7, {0, 3, 4, 6, 17, 20, 10},		9, {0,1,2, 2,1,3, 4,5,6}},
		{7, {2, 12, 10, 20, 13, 3, 6},		9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {6, 4, 20, 10, 2, 3},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}}, 
		
		{6, {14,7,15, 17,20,10}, 			6, {0,1,2, 3,4,5}},
		{9, {0,1,4, 14,7,15, 17,20,10}, 	9, {0,1,2, 3,4,5, 6,7,8}},
		{5, {10, 7, 20, 15, 12},			9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {15, 1, 7, 4, 10, 20},			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{7, {13, 14, 6, 7, 17, 20, 10},		9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {0, 2, 4, 7, 6, 17, 20, 10},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{6, {12, 20, 10, 7, 6, 13},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {20, 10, 4, 7, 6},				9, {0,1,2, 2,1,3, 2,3,4}},

		// 32 + 16 = 48
		{4, {17, 16, 10, 9},				6, {0,1,2, 2,1,3}}, 
		{5, {17, 0, 10, 1, 9},				9, {0,1,2, 2,1,3, 2,3,4}},
		{5, {12, 16, 2, 9, 10},				9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {2, 1, 10, 9},					6, {0,1,2, 2,1,3}},
		{7, {17, 16, 10, 9, 13, 3, 6},		9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {9, 10, 17, 0, 3, 6},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{8, {12, 16, 2, 9, 10, 13, 3, 6},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{5, {3, 6, 2, 9, 10}, 				9, {0,1,2, 2,1,3, 2,3,4}},
		
		{7, {17, 16, 10, 9, 14, 15, 7},		9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {17, 0, 10, 1, 9, 14, 7, 15},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{6, {9, 10, 7, 15, 13, 16},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {7, 3, 10, 1, 9},				9, {0,1,2, 2,1,3, 2,3,4}},
		{8, {17, 16, 10, 9, 13, 14, 6, 7}, 	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{7, {7, 6, 10, 9, 0, 14, 17},		9,{0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): Maybe problematic case 61: pipe-like, leaves hole on surface
		{7, {7, 6, 10, 9, 12, 16, 13},		9,{0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): Maybe problematic case 62: pipe-like, leaves hole on surface
		{4, {7, 6, 10, 9},					6, {0,1,2, 2,1,3}},					// Todo(Leo): 63 causes problems with some other case

		// 64
		{3, {18, 11, 21},						3, {0,1,2}},
		{6, {0, 1, 4, 18, 11, 21}, 				6, {0,1,2, 3,4,5}},
		{6, {12, 5, 2, 18, 11, 21},				6, {0,1,2, 3,4,5}},
		{7, {2, 1, 5, 4, 18, 11, 21},			9, {0,1,2, 2,1,3, 4,5,6}},
		{4, {13, 3, 21, 11},					6, {0,1,2, 2,1,3}},
		{5, {21, 4, 11, 0, 3},					9, {0,1,2, 2,1,3, 2,3,4}},
		{7, {13, 3, 21, 11, 12, 5, 2},			9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {4, 5, 2, 3, 11, 21},				12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{6, {14, 7, 15, 18, 11, 21},			6, {0,1,2, 3,4,5}},
		{9, {0, 1, 4, 14, 7, 15, 18, 11, 21,},	9, {0,1,2, 3,4,5, 6,7,8}},
		{7, {15, 12, 7, 5, 18, 11, 21},			9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {15, 1, 7, 4, 5, 18, 11, 21},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{5, {7, 11, 14, 21, 13},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {0, 14, 4, 7, 21, 11},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{6, {5, 7, 11, 21, 13, 12},				12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {11, 21, 7, 4, 5},					9, {0,1,2, 2,1,3, 2,3,4}},
 	
 		// 64 + 16 = 80
		{4, {16, 18, 8, 11},				6, {0,1,2, 2,1,3}},
		{5, {1, 18, 0, 11, 8},				9, {0,1,2, 2,1,3, 2,3,4}},
		{7, {16, 18, 8, 11, 12, 5, 2}, 		9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {2, 1, 18, 11, 8, 5},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {16, 13, 8, 3, 11}, 			9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {0, 3, 8, 11},					6, {0,1,2, 2,1,3}},
		{8, {16, 13, 8, 3, 11, 12, 5, 2},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{5, {5, 2, 8, 3, 11},				9, {0,1,2, 2,1,3, 2,3,4}},
		{7, {16, 18, 8, 11, 14, 7, 15},		9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {1, 18, 0, 11, 8, 14, 7, 15}, 	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{8, {15, 12, 7, 5, 16, 18, 8, 11},	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{7, {5, 7, 8, 11, 13, 6, 3},		9, {0,1,2, 2,1,3, 4,5,6}}, 			// Todo(Leo): maybe problematic case 91: pipe-like, leaves hole on one end
		{6, {13, 14, 7, 11, 8, 16},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {14, 7, 0, 11, 8},				9, {0,1,2, 2,1,3, 2,3,4}},
		{7, {5, 7, 8, 11, 12, 16, 13},		9, {0,1,2, 2,1,3, 4,5,6}}, 			// Todo(Leo): maybe problematic case 94: pipe-like, leaves hole on one end
		{4, {5, 7, 8, 11},					6, {0,1,2, 2,1,3}},

		// 64 + 32 = 96
		{6, {17, 20, 10, 18, 11, 21},						6, {0,1,2, 3,4,5}},
		{9, {0, 1, 4, 17, 20, 10, 18, 11, 21,}, 			9, {0,1,2, 3,4,5, 6,7,8}},
		{7, {2, 12, 10, 20, 18, 11, 21},					9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {4, 20, 1, 10, 2, 18, 11, 21},					12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {13, 3, 21, 11, 17, 20, 10},					9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {21, 4, 11, 0, 3, 17, 20, 10},					12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{8, {2, 12, 10, 20, 13, 3, 21, 11},					12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{7, {2, 3, 10, 11, 4, 20, 21},						9, {0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): maybe problematic case 103: pipe-like, leaves hole on one end
		
		{9, {14, 7, 15, 17, 20, 10, 18, 11, 21}, 			9, {0,1,2, 3,4,5, 6,7,8}},
		{12,{0, 1, 4, 14, 7, 15, 17, 20, 10, 18, 11, 21}, 	12,{0,1,2, 3,4,5, 6,7,8, 9,10,11}},
		{8, {10, 7, 20, 15, 12, 18, 11, 21},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{9, {1, 18, 15, 4, 20, 21, 7, 10, 11},				9, {0,1,2, 3,4,5, 6,7,8}},
		{8, {7, 11, 14, 21, 13, 17, 20, 10},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{9, {0, 14, 17, 4, 20, 21, 7, 11, 10},				9, {0,1,2, 3,4,5, 5,6,7}},
		{7, {13, 12, 21, 20, 7, 11, 10},					9, {0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): maybe problematic case 110: pipe-like, leaves hole on one end
		{6, {4, 20, 21, 7, 11, 10},							6, {0,1,2, 3,4,5}},

		// 64 + 32 + 16 = 112
		{5, {11, 10, 18, 17, 16},	 			9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {11, 10, 18, 17, 1, 0},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{6, {16, 18, 11, 10, 2, 13},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {18, 11, 1, 10, 2},					9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {3, 11, 10, 17, 16, 13},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{5, {10, 17, 11, 0, 3},					9, {0,1,2, 2,1,3, 2,3,4}},
		{7, {2, 3, 10, 11, 12, 16, 13},			9, {0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): maybe problematic case 118: pipe-like, leaves hole on one end
		{4, {2, 3, 10, 11},						6, {0,1,2, 2,1,3}},
		{8, {11, 10, 18, 17, 16, 14, 7, 15},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{9, {0, 14, 17, 1, 18, 15, 7, 11, 10},	9, {0,1,2, 3,4,5, 6,7,8}},
		{7, {15, 13, 18, 16, 7, 11, 10},		9, {0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): maybe problematic case 122: pipe-like, leaves hole on one end
		{6, {1, 18, 15, 7, 11, 10},				6, {0,1,2, 2,1,3}},
		{7, {13, 14, 16, 17, 7, 11, 10},		9, {0,1,2, 2,1,3, 4,5,6}},			// Todo(Leo): maybe problematic case 124: pipe-like, leaves hole on one end
		{6, {0, 14, 17, 7, 11, 10},				6, {0,1,2, 3,4,5}},
		{6, {12, 16, 13, 7, 11, 10}, 			6, {0,1,2, 3,4,5}},
		{3, {7, 11, 10},						3, {0,1,2}},

		/// 128
		{3, {19, 22, 23},						3, {0,1,2}},
		{6, {0, 1, 4, 19, 22, 23}, 				6, {0,1,2, 3,4,5}},
		{6, {12, 5, 2, 19, 22, 23},				6, {0,1,2, 3,4,5}},
		{7, {2, 1, 5, 4, 19, 22, 23},			9, {0,1,2, 2,1,3, 4,5,6}},
		{6, {13, 3, 6, 19, 22, 23},				6, {0,1,2, 3,4,5}},
		{7, {0, 3, 4, 6, 19, 22, 23},			9, {0,1,2, 2,1,3, 4,5,6}},
		{9, {12, 5, 2, 13, 3, 6, 19, 22, 23},	9, {0,1,2, 3,4,5, 6,7,8}},
		{8, {2, 3, 5, 6, 4, 19, 22, 23},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		
		{4, {15, 14, 23, 22},					6, {0,1,2, 2,1,3}},
		{7, {15, 14, 23, 22, 0, 1, 4},			9, {0,1,2, 2,1,3, 4,5,6}},
		{5, {5, 22, 12, 23, 15}, 				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {4, 5, 22, 23, 15, 1},				12,{0,1,2, 2,3,4 ,4,5,0, 0,2,4}},
		{5, {23, 6, 22, 13, 14},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {14, 22, 23, 6, 4, 0},				12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{6, {13, 12, 6, 5, 23, 22},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{5, {22, 23, 5, 6, 4},					9, {0,1,2, 2,1,3, 2,3,4}},

		/// 128 + 16 = 144
		{6, {16, 9, 8, 19, 22, 23},						6, {0,1,2, 3,4,5}},
		{7, {0, 1, 8, 9, 19, 22, 23},					9, {0,1,2, 2,1,3, 4,5,6}},
		{9, {12, 5, 2, 16, 9, 8, 19, 22, 23},			9, {0,1,2, 3,4,5, 6,7,8}},
		{8, {8, 5, 9, 2, 1, 19, 22, 23},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{9, {13, 3, 6, 16, 9, 8, 19, 22, 23},			9, {0,1,2, 3,4,5, 6,7,8}},
		{8, {6, 9, 3, 8, 0, 19, 22, 23},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{12,{12, 5, 2, 13, 3, 6, 16, 9, 8, 19, 22, 23}, 12,{0,1,2, 3,4,5, 6,7,8, 9,10,11}},
		{9, {2, 3, 19, 5, 20, 8, 6, 9, 23},				9, {0,1,2, 3,4,5, 6,7,8}},
		
		{7, {15, 14, 23, 22, 16, 9, 8},					9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {0, 1, 8, 9, 15, 14, 23, 22},				12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{8, {5, 22, 12, 23, 15, 16, 9, 8},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {15, 1, 23, 9, 0, 14, 17},					9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 155
		{8, {23, 6, 22, 13, 14, 16, 9, 8},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {0, 14, 8, 22, 6, 9, 23},					9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 157
		{9, {12, 16, 13, 5, 22, 8, 6, 9, 23},			9, {0,1,2, 3,4,5, 6,7,8}},
		{6, {5, 22, 8, 6, 9, 23},						6, {0,1,2, 3,4,5}},

		/// 128 + 32 = 160
		{4, {19, 17, 23, 20},				6, {0,1,2, 2,1,3}},
		{7, {19, 17, 23, 20, 0, 1, 4}, 		9, {0,1,2, 2,1,3, 4,5,6}},
		{5, {19, 2, 23, 12, 20},			9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {2, 1, 4, 20, 23, 19},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {19, 17, 23, 20, 13, 3, 6},		9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {0, 3, 4, 6, 19, 17, 23, 20},	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{8, {19, 2, 23, 12, 20, 13, 3, 6},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {6, 4, 23, 20, 2, 3, 19},		9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 167
		{5, {14, 17, 15, 20, 23},			9, {0,1,2, 2,1,3, 2,3,4}},
		{8, {14, 17, 15, 20, 23, 0, 1, 4},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{4, {15, 12, 23, 20},				6, {0,1,2, 2,1,3}},
		{5, {1, 4, 15, 20, 23},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {13, 14, 17, 20, 23, 6},		12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {6, 4, 23, 20, 0, 14, 17},		9, {0,1,2, 2,1,3, 4,5,6}}, 				// Todo(Leo): suspicious half pipe thing 173
		{5, {6, 13, 23, 12, 20},			9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {6, 4, 23, 20},					6, {0,1,2, 2,1,3}},

		/// 128 + 32 + 16
		{5, {9, 23, 16, 19, 17}, 				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {1, 9, 23, 19, 17, 0},				12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{6, {9, 23, 16, 19, 12, 2},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{5, {23, 19, 9, 2, 1},					9, {0,1,2, 2,1,3, 2,3,4}},
		{8, {9, 23, 16, 19, 17, 13, 3, 6},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {0, 3, 17, 19, 6, 9, 23},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 181
		{9, {12, 16, 13, 2, 3, 19, 6, 9, 23},	9, {0,1,2, 3,4,5, 6,7,8}},
		{6, {2, 3, 19, 6, 9, 23},				6, {0,1,2, 3,4,5}},
		{6, {14, 22, 8, 16, 18, 15},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {15, 1, 23, 9, 0, 14, 17},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 185
		{5, {16, 9, 12, 23, 15},				9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {15, 1, 23, 9},						6, {0,1,2, 2,1,3}},
		{7, {13, 14, 16, 17, 6, 9, 23},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 188
		{6, {0, 14, 17, 6, 9, 23},				6, {0,1,2, 3,4,5}},
		{6, {12, 16, 13, 6, 9, 23},				6, {0,1,2, 3,4,5}},
		{3, {6, 9, 23},							3, {0,1,2}},

		/// 128 + 64 = 192
		{4, {18, 19, 21, 22}, 				6, {0,1,2, 2,1,3}},
		{7, {18, 19, 21, 22, 0, 1, 4}, 		9, {0,1,2, 2,1,3, 4,5,6}},
		{7, {18, 19, 21, 22, 13, 5, 2},		9, {0,1,2, 2,1,3, 4,5,6}},
		{8, {2, 1, 5, 4, 18, 19, 21, 22},	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}},
		{5, {3, 19, 13, 22, 21},			9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {22, 21, 4, 1, 3, 19},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{8, {3, 19, 13, 22, 21, 12, 5, 2},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {4, 5, 21, 22, 14, 19, 15},		9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 199
		
		{5, {18, 15, 21, 14, 22},			9, {0,1,2, 2,1,3, 2,3,4}},
		{8, {18, 15, 21, 14, 22, 0, 1, 4},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{6, {15, 12, 5, 22, 21, 18},		12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {4, 5, 21, 22, 1, 18, 15}, 		9, {0,1,2, 2,1,3, 4,5,6}}, 				// Todo(Leo): suspicious half pipe thing 203
		{4, {13, 14, 21, 22},				6, {0,1,2, 2,1,3}},
		{5, {4, 0, 21, 14, 22},				9, {0,1,2, 2,1,3, 2,3,4}},
		{5, {12, 17, 13, 22, 21},			9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {4, 5, 21, 22},					6, {0,1,2, 2,1,3}},

		/// 128 + 64 + 16 = 208
		{5, {22, 8, 19, 16, 18}, 				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {8, 0, 1, 18, 19, 22},				12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{8, {22, 8, 19, 16, 18, 13, 5, 2},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{7, {2, 1, 19, 18, 5, 22, 8},			9, {0,1,2, 2,1,3, 4,5,6}}, 				// Todo(Leo): suspicious half pipe thing 211
		{6, {22, 8, 19, 16, 3, 13},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{5, {19, 23, 3, 8, 0}, 					9, {0,1,2, 2,1,3, 2,3,4}},
		{9, {12, 16, 13, 2, 3, 19, 5 ,22, 8},	9, {0,1,2, 3,4,5, 5,6,7}},
		{6, {2, 3, 19, 5, 22, 8},				6, {0,1,2, 3,4,5}},
		{6, {14, 22, 8, 16, 18, 15},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {0, 14, 8, 22, 1, 18, 15},			9, {0,1,2, 2,1,3, 4,5,6}}, 				// Todo(Leo): suspicious half pipe thing 217
		{7, {15, 13, 18, 16, 5, 22, 8},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 218
		{6, {1, 18, 15, 5, 22, 8},				6, {0,1,2, 3,4,5}},
		{5, {8, 16, 22, 13, 14},				9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {0, 14, 8, 22},						6, {0,1,2, 2,1,3}},
		{6, {12, 16, 13, 5, 22, 8},				6, {0,1,2, 2,1,3}},
		{3, {5, 22, 8},							3, {0,1,2}},

		/// 128 + 64 + 32 = 224
		{5, {20, 21, 17 ,18, 19},				9, {0,1,2, 2,1,3, 2,3,4}},
		{8, {20, 21, 17, 18, 19, 0, 1, 4},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}},
		{6, {12, 20, 21, 18, 19, 2},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {2, 1, 19, 18, 4, 20, 21},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 227
		{6, {19, 17, 20, 21, 13, 3},			12,{0,1,2, 2,3,4, 4,5,0, 0,2,4}},
		{7, {0, 3, 17, 19, 4, 20, 21},			9, {0,1,2, 2,1,3, 4,5,5}},				// Todo(Leo): suspicious half pipe thing 229
		{7, {13, 12, 21, 20, 2, 3, 19},			9, {0,1,2, 2,1,3, 4,5,6}},				// Todo(Leo): suspicious half pipe thing 230
		{6, {2, 3, 19, 4, 20, 21},				6, {0,1,2, 2,1,3}},
		
		{6, {15, 14, 18, 17, 21, 20},			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}},
		{9, {0, 14, 15, 1, 18, 15, 16, 20, 2},	9, {0,1,2, 3,4,5, 6,7,8}},
		{5, {21, 18, 20, 15, 12},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {1, 18, 15, 4, 20, 21},				6, {0,1,2, 3,4,5}},
		{5, {17, 20, 14, 21, 13},				9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {0, 14, 15, 4, 20, 21},				6, {0,1,2, 3,4,5}},
		{4, {13, 12, 21, 20},					6, {0,1,2, 2,1,3}},
		{3, {4, 20, 21},						3, {0,1,2}},

		/// 128 + 64 + 32 + 16 = 240
		{4, {17, 16, 19, 18}, 		6, {0,1,2, 2,1,3}},
		{5, {0, 1, 17, 18, 19},		9, {0,1,2, 2,1,3, 2,3,4}},
		{5, {2, 12, 19, 16, 18},	9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {2, 1, 19, 18},			6, {0,1,2, 2,1,3}},
		{5, {13, 3, 16, 19, 17},	9, {0,1,2, 2,1,3, 2,3,4}},
		{4, {0, 3, 17, 19},			6, {0,1,2, 2,1,3}},
		{6, {12, 16, 13, 2, 3, 19}, 6, {0,1,2, 3,4,5}},
		{3, {2, 3, 19},				3, {0,1,2}},
		{5, {15, 14, 18, 17, 16},	9, {0,1,2, 2,1,3, 2,3,4}},
		{6, {0, 14, 17, 1, 18, 15},	6, {0,1,2, 3,4,5}},
		{4, {15, 12, 18, 16},		6, {0,1,2, 2,1,3}},
		{3, {1, 18, 15},			3, {0,1,2}},
		{4, {13, 14, 16, 17},		6, {0,1,2, 2,1,3}},
		{3, {0, 14, 17},			3, {0,1,2}},
		{3, {12, 16, 13},			3, {0,1,2}},
		{},
	};

	s32 zCount = 0;

	for (f32 z = -1; z < fieldSize.z; z += gridScale)
	{
		f32 Z = z + gridScale;
		zCount += 1;

		for (f32 y = -1; y < fieldSize.y; y += gridScale)
		{
			f32 Y = y + gridScale;

			// Note(Leo): this mostly cuts sampling in half
			f32 previous_f002 = sample_field({-1, y, z}, fieldData);
			f32 previous_f008 = sample_field({-1, Y, z}, fieldData);
			f32 previous_f032 = sample_field({-1, y, Z}, fieldData);
			f32 previous_f128 = sample_field({-1, Y, Z}, fieldData);

			for (f32 x = -1; x < fieldSize.x; x += gridScale)
			{
				f32 X = x + gridScale;

				f32 f001 = previous_f002;
				f32 f002 = sample_field({X, y, z}, fieldData);
				f32 f004 = previous_f008;
				f32 f008 = sample_field({X, Y, z}, fieldData);
				f32 f016 = previous_f032;
				f32 f032 = sample_field({X, y, Z}, fieldData);
				f32 f064 = previous_f128;
				f32 f128 = sample_field({X, Y, Z}, fieldData);

				previous_f002 = f002;
				previous_f008 = f008;
				previous_f032 = f032;
				previous_f128 = f128;

				u8 caseId 	= 1 * (f001 < 0);
				caseId 		+= 2 * (f002 < 0);
				caseId 		+= 4 * (f004 < 0);
				caseId 		+= 8 * (f008 < 0);
				caseId 		+= 16 * (f016 < 0);
				caseId 		+= 32 * (f032 < 0);
				caseId 		+= 64 * (f064 < 0);
				caseId 		+= 128 * (f128 < 0);

				constexpr f32 h = -0.5;
				constexpr f32 H = 0.5;

				constexpr v3 v001 = {h,h,h};
				constexpr v3 v002 = {H,h,h};
				constexpr v3 v004 = {h,H,h};
				constexpr v3 v008 = {H,H,h};
				constexpr v3 v016 = {h,h,H};
				constexpr v3 v032 = {H,h,H};
				constexpr v3 v064 = {h,H,H};
				constexpr v3 v128 = {H,H,H};

				constexpr v3 gridVertices [] = {v001, v002, v004, v008, v016, v032, v064, v128};
				f32 gridValues [] 	= {f001, f002, f004, f008, f016, f032, f064, f128};

				BetterEdgeStruct const & c = betterCases[caseId];
				for (s32 i = 0; i < c.vertexCount; ++i)
				{
					s32 edgeId = c.edgeIdIds[i];

					s32 e0 = edges[edgeId][0];
					s32 e1 = edges[edgeId][1];

				 	constexpr f32 isoLevel = 0;
				 	f32 t 		= (isoLevel - gridValues[e0]) / (gridValues[e1] - gridValues[e0]);
				 	v3 position = v3_lerp(gridVertices[e0], gridVertices[e1], t);

					vertices[vertexCount + i].position = v3{x,y,z} + gridScale * position;
					vertices[vertexCount + i].texCoord = vertices[vertexCount + i].position.xy;
				}

				for (s32 i = 0; i < c.triangleIndexCount; ++i)
				{
					Assert(c.indices[i] < c.vertexCount);
					indices[indexCount + i] = vertexCount + c.indices[i];
				}

				vertexCount += c.vertexCount;
				indexCount += c.triangleIndexCount;

				if (c.vertexCount == 0 && caseId != 0 && caseId != 255)
				{
					logDebug(0) << "Missing case: " << (u32)caseId;
				}
			}
		}
	}

	// logDebug(0) << "zCount = " << zCount;

	// logDebug(0) << "Marching Cubes Took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";
	// logDebug(0) << "Vertices: " << vertexCount << "/" << vertexCapacity << ", indices: " << indexCount << "/" << indexCapacity;

	mesh_generate_normals(vertexCount, vertices, indexCount, indices);
	mesh_generate_tangents(vertexCount, vertices, indexCount, indices);

	*outVertexCount = vertexCount;
	*outIndexCount = indexCount;
}