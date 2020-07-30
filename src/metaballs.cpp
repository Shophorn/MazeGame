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

v3 fieldResolution = {7,5,7};

// https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
float opSmoothUnion( float d1, float d2, float k )
{
    float h = clamp_f32(0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f);
    return interpolate(d2, d1, h) - k*h*(1.0f-h);
}

internal f32 sample_field (v3 pos)
{
	v3 pos1 = {3,2,2};
	v3 pos2 = {4,3,4};
	v3 pos3 = {1,1,1};
	v3 pos4 = {4.5, 2.5, 3};

	f32 r1 = 2;
	f32 r2 = 1;
	f32 r3 = 1.5;
	f32 r4 = 1.2;

	f32 d1 = magnitude_v3(pos - pos1) - r1;
	f32 d2 = magnitude_v3(pos - pos2) - r2;
	f32 d3 = magnitude_v3(pos - pos3) - r3;
	f32 d4 = magnitude_v3(pos - pos4) - r4;

	f32 result = opSmoothUnion(d1, d2, 0.5f);
	result = opSmoothUnion(result, d3, 0.5f);
	result = opSmoothUnion(result, d4, 0.5f);

	return result;

	return min_f32(min_f32(d2, d3), d4);
};

internal MeshHandle generate_metaball()
{
	s32 vertexCountPerCube 	= 14;
	s32 indexCountPerCube 	= 36;
	s32 cubeCapacity 		= 100000;
	s32 vertexCapacity 		= vertexCountPerCube * cubeCapacity;
	s32 indexCapacity 		= indexCountPerCube * cubeCapacity;

	Vertex * vertices 	= push_memory<Vertex>(*global_transientMemory, vertexCapacity, ALLOC_NO_CLEAR);
	u16 * indices 		= push_memory<u16>(*global_transientMemory, indexCapacity, ALLOC_NO_CLEAR);

	u32 vertexCount = 0;
	u32 indexCount = 0;

	PlatformTimePoint startTime = platformApi->current_time();

	f32 scale = 0.2;

	s32 edges [24][2] = 
	{
		{0,1}, {0,2}, {1,3}, {2,3}, {0,4}, {1,5}, {2,6}, {3,7}, {4,5}, {4,6}, {5,7}, {6,7},
		{1,0}, {2,0}, {3,1}, {3,2}, {4,0}, {5,1}, {6,2}, {7,3}, {5,4}, {6,4}, {7,5}, {7,6},
	};

	struct BetterEdgeStruct
	{
		s32 vertexCount;
		s32 edgeIdIds[12];
		s32 triangleIndexCount;
		s32 indices[18];
	};

	BetterEdgeStruct betterCases [256] = {};

	betterCases[0] = {};
	betterCases[1] = {3, {0,1,4},				3, {0,1,2}};
	betterCases[2] = {3, {12, 5, 2,}, 			3, {0,1,2}};
	betterCases[3] = {4, {2, 1, 5, 4},			6, {0,1,2, 2,1,3}};
	betterCases[4] = {3, {13, 3, 6},			3, {0,1,2}};
	betterCases[5] = {4, {0, 3, 4, 6},			6, {0,1,2, 2,1,3}};
	betterCases[6] = {6, {12, 5, 2, 1, 3, 6},	6, {0,1,2, 3,4,5}};
	betterCases[7] = {5, {2, 15, 5, 6, 4},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[8] = {3, {14, 7, 15},			3, {0,1,2}};
	betterCases[9] = {6, {0, 1, 4, 14, 7, 15},	6, {0,1,2, 3,4,5}};
	betterCases[10] = {4, {15, 12, 7, 5},		6, {0,1,2, 2,1,3}};
	betterCases[11] = {5, {15, 1, 7, 4, 5},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[12] = {4, {1, 2, 6, 7},			6, {0,1,2, 2,1,3}};
	betterCases[13] = {5, {0, 2, 4, 7, 6},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[14] = {5, {13, 12, 6, 5, 7},	9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[15] = {4, {4, 5, 6, 7},			6, {0,1,2, 2,1,3}};

	betterCases[16] = {3, {16, 9, 8}, 						3, {0,1,2}};
	betterCases[17] = {4, {0, 1, 8, 9}, 					6, {0,1,2, 2,1,3}};
	betterCases[18] = {6, {16, 9, 8, 13, 5, 2}, 			6, {0,1,2, 3,4,5}};
	betterCases[19] = {5, {8, 5, 9, 2, 1},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[20] = {6, {13, 3, 6, 16, 9, 8},				6, {0,1,2, 3,4,5}};
	betterCases[21] = {5, {6, 9, 3, 8, 0},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[22] = {9, {12, 5, 2, 1, 3, 6, 16, 9, 8}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[23] = {6, {2, 3, 5, 6, 8, 9},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[24] = {6, {14, 7, 15, 16, 9, 8},			6, {0,1,2, 3,4,5}};
	betterCases[25] = {7, {0, 1, 8, 9, 14, 7, 15},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[26] = {7, {15, 12, 7, 5, 16, 9, 8},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[27] = {/* TODO, lärppä*/};
	betterCases[28] = {7, {1, 2, 6, 7, 16, 9, 7},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[29] = {/*TODO, lärppä*/};
	betterCases[30] = {8, {13, 12,6, 5, 7, 16, 9, 8},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[31] = {5, {9, 8, 6, 5, 7}, 					9, {0,1,2, 2,1,3, 2,3,4}};

	betterCases[32] = {3, {17, 8, 10},					3, {0,1,2}};
	betterCases[33] = {6, {0, 1, 4, 17, 10, 22}, 		6, {0,1,2, 3,4,5}};
	betterCases[34] = {4, {2, 12, 10, 20}, 				6, {0,1,2, 2,1,3}};
	betterCases[35]	= {5, {4, 20, 1, 10, 2},			9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[36]	= {6, {13, 3, 6, 17, 20, 10},		6, {0,1,2, 3,4,5}};
	betterCases[37]	= {7, {0, 3, 4, 6, 17, 20, 10},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[38]	= {7, {2, 12, 10, 20, 13, 3, 6},	9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[39]	= {/*TODO*/}; 
	betterCases[40]	= {6, {14,7,15, 17,20,10}, 			6, {0,1,2, 3,4,5}};
	betterCases[41]	= {9, {0,1,4, 14,7,15, 17,20,10}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[42]	= {5, {10, 7, 20, 15, 12},			9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[43]	= {6, {15, 1, 7, 4, 10, 20},		12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[44]	= {7, {13, 14, 6, 7, 17, 20, 10},	9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[45]	= {8, {0, 2, 4, 7, 6, 17, 20, 10},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[46]	= {/*TODO*/};
	betterCases[47]	= {5, {20, 10, 4, 7, 6},			9, {0,1,2, 2,1,3, 2,3,4}};

	betterCases[48]	= {4, {17, 16, 10, 9},					6, {0,1,2, 2,1,3}}; 
	betterCases[49]	= {5, {17, 0, 10, 1, 9},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[50]	= {5, {12, 16, 2, 9, 10},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[51]	= {4, {2, 1, 10, 9},					6, {0,1,2, 2,1,3}};
	betterCases[52]	= {7, {17, 16, 10, 9, 13, 3, 6},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[53]	= {/*TODO, lärppä*/};
	betterCases[54]	= {8, {12, 16, 2, 9, 10, 13, 3, 6},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[55]	= {5, {3, 6, 2, 9, 10}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[56]	= {7, {17, 16, 10, 9, 14, 15, 7},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[57]	= {8, {17, 0, 10, 1, 9, 14, 15, 7},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[58]	= {/*TODO, lärppä*/};
	betterCases[59]	= {5, {7, 3, 10, 1, 9},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[60]	= {8, {17, 16, 10, 9, 13, 14, 6, 7}, 	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	betterCases[61]	= {7, {7, 6, 10, 9, 0, 14, 17},			9,{0,1,2, 2,1,3, 4,5,6}};			// Todo(Leo): Maybe problematic case 61: pipe-like, leaves hole on surface
	betterCases[62]	= {7, {7, 6, 10, 9, 12, 16, 13},		9,{0,1,2, 2,1,3, 4,5,6}};			// Todo(Leo): Maybe problematic case 62: pipe-like, leaves hole on surface
	// Todo(Leo): This causes problems with other things
	betterCases[63]	= {4, {7, 6, 10, 9},					6, {0,1,2, 2,1,3}};

	betterCases[64]	= {3, {18, 11, 21},							3, {0,1,2}};
	betterCases[65]	= {6, {0, 1, 4, 18, 11, 21}, 				6, {0,1,2, 3,4,5}};
	betterCases[66]	= {6, {12, 5, 2, 18, 11, 21},				6, {0,1,2, 3,4,5}};
	betterCases[67]	= {7, {2, 1, 5, 4, 18, 11, 21},				9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[68]	= {4, {13, 3, 21, 11},						6, {0,1,2, 2,1,3}};
	betterCases[69]	= {5, {21, 4, 11, 0, 3},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[70]	= {7, {13, 3, 21, 11, 12, 5, 2},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[71]	= {/*TODO, lärppä*/};
	betterCases[72]	= {6, {14, 7, 15, 18, 11, 21},				6, {0,1,2, 3,4,5}};
	betterCases[73]	= {9, {0, 1, 4, 14, 7, 15, 18, 11, 21,},	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[74]	= {7, {15, 12, 7, 5, 18, 11, 21},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[75]	= {8, {15, 1, 7, 4, 5, 18, 11, 21},			12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[76]	= {5, {7, 11, 14, 21, 13},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[77]	= {6, {0, 14, 4, 7, 21, 11},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[78]	= {/*TODO, lärppä*/};
	betterCases[79]	= {5, {11, 21, 7, 4, 5},					9, {0,1,2, 2,1,3, 2,3,4}};

	betterCases[80]	= {4, {16, 18, 8, 11},					6, {0,1,2, 2,1,3}};
	betterCases[81]	= {5, {1, 18, 0, 11, 8},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[82]	= {7, {16, 18, 8, 11, 12, 5, 2}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[83]	= {/*TODO, lärppä*/};
	betterCases[84]	= {5, {16, 13, 8, 3, 11}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[85]	= {4, {0, 3, 8, 11},					6, {0,1,2, 2,1,3}};
	betterCases[86]	= {8, {16, 13, 8, 3, 11, 12, 5, 2},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[87]	= {5, {5, 2, 8, 3, 11},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[88]	= {7, {16, 18, 8, 11, 14, 7, 15},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[89]	= {8, {1, 18, 0, 11, 8, 14, 7, 15}, 	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[90]	= {8, {15, 12, 7, 5, 16, 18, 8, 11},	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	betterCases[91]	= {7, {5, 7, 8, 11, 13, 6, 3},			9, {0,1,2, 2,1,3, 4,5,6}}; 			// Todo(Leo): maybe problematic case 91: pipe-like, leaves hole on one end
	betterCases[92]	= {/*TODO, lärppä*/};
	betterCases[93]	= {5, {14, 7, 0, 11, 8},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[94]	= {7, {5, 7, 8, 11, 12, 16, 13},		9, {0,1,2, 2,1,3, 4,5,6}}; 			// Todo(Leo): maybe problematic case 94: pipe-like, leaves hole on one end
	betterCases[95]	= {4, {5, 7, 8, 11},					6, {0,1,2, 2,1,3}};

	betterCases[96]	= {6, {17, 20, 10, 18, 11, 21},							6, {0,1,2, 3,4,5}};
	betterCases[97]	= {9, {0, 1, 4, 17, 20, 10, 18, 11, 21,}, 				9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[98]	= {7, {2, 12, 10, 20, 18, 11, 21},						9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[99]	= {8, {4, 20, 1, 10, 2, 18, 11, 21},					12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[100]	= {7, {13, 3, 21, 11, 17, 20, 10},					9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[101]	= {8, {21, 4, 11, 0, 3, 17, 20, 10},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[102]	= {8, {2, 12, 10, 20, 13, 3, 21, 11},				12,{0,1,2, 2,1,3, 4,5,6, 6,5,6}};
	// Todo(Leo): maybe problematic case 103: pipe-like, leaves hole on one end
	betterCases[103]	= {7, {2, 3, 10, 11, 4, 20, 21},					9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[104]	= {9, {14, 7, 15, 17, 20, 10, 18, 11, 21}, 			9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[105]	= {12,{0, 1, 4, 14, 7, 15, 17, 20, 10, 18, 11, 21}, 12,{0,1,2, 3,4,5, 6,7,8, 9,10,11}};
	betterCases[106]	= {8, {10, 7, 20, 15, 12, 18, 11, 21},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	// Todo(Leo): maybe problematic case 107: three separate inverse corners
	betterCases[107]	= {9, {1, 18, 15, 4, 20, 21, 7, 10, 11},			9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[108]	= {8, {7, 11, 14, 21, 13, 17, 20, 10},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	// Todo(Leo): maybe problematic case 109: three separate inverse corners
	betterCases[109]	= {9, {0, 14, 17, 4, 20, 21, 7, 11, 10},			9, {0,1,2, 3,4,5, 5,6,7}};
	// Todo(Leo): maybe problematic case 110: pipe-like, leaves hole on one end
	betterCases[110]	= {7, {13, 12, 21, 20, 7, 11, 10},					9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[111]	= {6, {4, 20, 21, 7, 11, 10},						6, {0,1,2, 3,4,5}};

	// 64 + 32 + 16
	betterCases[112]	= {5, {11, 10, 18, 17, 16},	 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[113]	= {6, {11, 10, 18, 17, 1, 0},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[114]	= {/*TODO, lärppä*/};
	betterCases[115]	= {5, {18, 11, 1, 10, 2},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[116]	= {/*TODO, tyhmä lärppä*/};
	betterCases[117]	= {5, {10, 17, 11, 0, 3},					9, {0,1,2, 2,1,3, 2,3,4}};
	// Todo(Leo): maybe problematic case 118: pipe-like, leaves hole on one end
	betterCases[118]	= {7, {2, 3, 10, 11, 12, 16, 13},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[119]	= {4, {2, 3, 10, 11},						6, {0,1,2, 2,1,3}};
	betterCases[120]	= {8, {11, 10, 18, 17, 16, 14, 7, 15},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[121]	= {9, {0, 14, 17, 1, 18, 15, 7, 11, 10},	9, {0,1,2, 3,4,5, 6,7,8}};
	// Todo(Leo): maybe problematic case 122: pipe-like, leaves hole on one end
	betterCases[122]	= {7, {15, 13, 18, 16, 7, 11, 10},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[123]	= {6, {1, 18, 15, 7, 11, 10},				6, {0,1,2, 2,1,3}};
	// Todo(Leo): maybe problematic case 124: pipe-like, leaves hole on one end
	betterCases[124]	= {7, {13, 14, 16, 17, 7, 11, 10},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[125]	= {6, {0, 14, 17, 7, 11, 10},				6, {0,1,2, 3,4,5}};
	betterCases[126]	= {6, {12, 16, 13, 7, 11, 10}, 				6, {0,1,2, 3,4,5}};
	betterCases[127]	= {3, {7, 11, 10},							3, {0,1,2}};

	/// 128
	betterCases[128]	= {3, {19, 22, 23},						3, {0,1,2}};
	betterCases[129]	= {6, {0, 1, 4, 19, 22, 23}, 			6, {0,1,2, 3,4,5}};
	betterCases[130]	= {6, {12, 5, 2, 19, 22, 23},			6, {0,1,2, 3,4,5}};
	betterCases[131]	= {7, {2, 1, 5, 4, 19, 22, 23},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[132]	= {6, {13, 3, 6, 19, 22, 23},			6, {0,1,2, 3,4,5}};
	betterCases[133]	= {7, {0, 3, 4, 6, 19, 22, 23},			9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[134]	= {9, {12, 5, 2, 13, 3, 6, 19, 22, 23},	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[135]	= {8, {2, 3, 5, 6, 4, 19, 22, 23},		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[136]	= {4, {15, 14, 23, 22},					6, {0,1,2, 2,1,3}};
	betterCases[137]	= {7, {15, 14, 23, 22, 0, 1, 4},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[138]	= {5, {5, 22, 12, 23, 15}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[139]	= {/*TODO, tyhmä lärppä*/};
	betterCases[140]	= {5, {23, 6, 22, 13, 14},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[141]	= {/*TODO, tyhmä lärppä*/};
	betterCases[142]	= {6, {13, 12, 6, 5, 23, 22},			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[143]	= {5, {22, 23, 5, 6, 4},	9, {0,1,2, 2,1,3, 2,3,4}};

	/// 128 + 16
	betterCases[144]	= {6, {16, 9, 8, 19, 22, 23},						6, {0,1,2, 3,4,5}};
	betterCases[145]	= {7, {0, 1, 8, 9, 19, 22, 23},						9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[146]	= {9, {12, 5, 2, 16, 9, 8, 19, 22, 23},				9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[147]	= {8, {8, 5, 9, 2, 1, 19, 22, 23},					12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[148]	= {9, {13, 3, 6, 16, 9, 8, 19, 22, 23},				9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[149]	= {8, {6, 9, 3, 8, 0, 19, 22, 23},					12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[150]	= {12,{12, 5, 2, 13, 3, 6, 16, 9, 8, 19, 22, 23}, 	12,{0,1,2, 3,4,5, 6,7,8, 9,10,11}};
	betterCases[151]	= {9, {2, 3, 19, 5, 20, 8, 6, 9, 23},				9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[152]	= {7, {15, 14, 23, 22, 16, 9, 8},					9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[153]	= {8, {0, 1, 8, 9, 15, 14, 23, 22},					12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	betterCases[154]	= {8, {5, 22, 12, 23, 15, 16, 9, 8},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[155]	= {7, {15, 1, 23, 9, 0, 14, 17},					9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 155
	betterCases[156]	= {8, {23, 6, 22, 13, 14, 16, 9, 8},				12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[157]	= {7, {0, 14, 8, 22, 6, 9, 23},						9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 157
	betterCases[158]	= {9, {12, 16, 13, 5, 22, 8, 6, 9, 23},				9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[159]	= {6, {5, 22, 8, 6, 9, 23},							6, {0,1,2, 3,4,5}};

	/// 128 + 32
	betterCases[160]	= {4, {19, 17, 23, 20},					6, {0,1,2, 2,1,3}};
	betterCases[161]	= {7, {19, 17, 23, 20, 0, 1, 4}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[162]	= {5, {19, 2, 23, 12, 20},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[163]	= {/*Todo, tyhmä lärppä*/};
	betterCases[164]	= {7, {19, 17, 23, 20, 13, 3, 6},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[165]	= {8, {0, 3, 4, 6, 19, 17, 23, 20},		12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	betterCases[166]	= {8, {19, 2, 23, 12, 20, 13, 3, 6},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[167]	= {7, {6, 4, 23, 20, 2, 3, 19},			9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 167
	betterCases[168]	= {5, {14, 17, 15, 20, 23},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[169]	= {8, {14, 17, 15, 20, 23, 0, 1, 4},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[170]	= {4, {15, 12, 23, 20},					6, {0,1,2, 2,1,3}};
	betterCases[171]	= {5, {1, 4, 15, 20, 23},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[172]	= {/*todo, tyhmä lärppä*/};
	betterCases[173]	= {7, {6, 4, 23, 20, 0, 14, 17},		9, {0,1,2, 2,1,3, 4,5,6}}; 				// Todo(Leo): suspicious half pipe thing 173
	betterCases[174]	= {5, {6, 13, 23, 12, 20},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[175]	= {4, {6, 4, 23, 20},					6, {0,1,2, 2,1,3}};

	/// 128 + 32 + 16
	betterCases[176]	= {5, {9, 23, 16, 19, 17}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[177]	= {/*todo, tyhmä lärppä*/};
	betterCases[178]	= {6, {9, 23, 16, 19, 12, 2},			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[179]	= {5, {23, 19, 9, 2, 1},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[180]	= {8, {9, 23, 16, 19, 17, 13, 3, 6},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[181]	= {7, {0, 3, 17, 19, 6, 9, 23},			9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 181
	betterCases[182]	= {9, {12, 16, 13, 2, 3, 19, 6, 9, 23},	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[183]	= {6, {2, 3, 19, 6, 9, 23},				6, {0,1,2, 3,4,5}};
	betterCases[184]	= {/*todo, tyhmä lärppä*/};
	betterCases[185]	= {7, {15, 1, 23, 9, 0, 14, 17},		9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 185
	betterCases[186]	= {5, {16, 9, 12, 23, 15},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[187]	= {4, {15, 1, 23, 9},					6, {0,1,2, 2,1,3}};
	betterCases[188]	= {7, {13, 14, 16, 17, 6, 9, 23},		9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 188
	betterCases[189]	= {6, {0, 14, 17, 6, 9, 23},			6, {0,1,2, 3,4,5}};
	betterCases[190]	= {6, {12, 16, 13, 6, 9, 23},			6, {0,1,2, 3,4,5}};
	betterCases[191]	= {3, {6, 9, 23},						3, {0,1,2}};

	/// 128 + 64
	betterCases[192]	= {4, {18, 19, 21, 22}, 				6, {0,1,2, 2,1,3}};
	betterCases[193]	= {7, {18, 19, 21, 22, 0, 1, 4}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[194]	= {7, {18, 19, 21, 22, 13, 5, 2},		9, {0,1,2, 2,1,3, 4,5,6}};
	betterCases[195]	= {8, {2, 1, 5, 4, 18, 19, 21, 22},		12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	betterCases[196]	= {5, {3, 19, 13, 22, 21},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[197]	= {/*todo, tyhmä lärppä*/};
	betterCases[198]	= {8, {3, 19, 13, 22, 21, 12, 5, 2},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[199]	= {7, {4, 5, 21, 22, 14, 19, 15},		9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 199
	betterCases[200]	= {5, {18, 15, 21, 14, 22},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[201]	= {8, {18, 15, 21, 14, 20, 0, 1, 4},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[202]	= {/*todo, tyhmä lärppä*/};
	betterCases[203]	= {7, {4, 5, 21, 22, 1, 18, 15}, 		9, {0,1,2, 2,1,3, 4,5,6}}; 				// Todo(Leo): suspicious half pipe thing 203
	betterCases[204]	= {4, {13, 14, 21, 22},					6, {0,1,2, 2,1,3}};
	betterCases[205]	= {5, {4, 0, 21, 14, 22},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[206]	= {5, {12, 17, 13, 22, 21},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[207]	= {4, {4, 5, 21, 22},					6, {0,1,2, 2,1,3}};

	/// 128 + 64 + 16
	betterCases[208]	= {5, {22, 8, 19, 16, 18}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[209]	= {/*Todo, tyhmä lärppä*/};
	betterCases[210]	= {8, {22, 8, 19, 16, 18, 13, 5, 2},	12, {0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[211]	= {7, {2, 1, 19, 18, 5, 22, 8},			9, {0,1,2, 2,1,3, 4,5,6}}; 				// Todo(Leo): suspicious half pipe thing 211
	betterCases[212]	= {6, {22, 8, 19, 16, 3, 13},			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[213]	= {5, {19, 23, 3, 8, 0}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[214]	= {9, {12, 16, 13, 2, 3, 19, 5 ,22, 8},	9, {0,1,2, 3,4,5, 5,6,7}};
	betterCases[215]	= {6, {2, 3, 19, 5, 22, 8},				6, {0,1,2, 3,4,5}};
	betterCases[216]	= {/*todo, tyhmä lärppä*/};
	betterCases[217]	= {7, {0, 14, 8, 22, 1, 18, 15},		9, {0,1,2, 2,1,3, 4,5,6}}; 				// Todo(Leo): suspicious half pipe thing 217
	betterCases[218]	= {7, {15, 13, 18, 16, 5, 22, 8},		9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 218
	betterCases[219]	= {6, {1, 18, 15, 5, 22, 8},			6, {0,1,2, 3,4,5}};
	betterCases[220]	= {5, {8, 16, 22, 13, 14},				9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[221]	= {4, {0, 14, 8, 22},					6, {0,1,2, 2,1,3}};
	betterCases[222]	= {6, {12, 16, 13, 5, 22, 8},			6, {0,1,2, 2,1,3}};
	betterCases[223]	= {3, {5, 22, 8},						3, {0,1,2}};

	/// 128 + 64 + 32
	betterCases[224]	= {5, {20, 21, 17 ,18, 19},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[225]	= {8, {20, 21, 17, 18, 19, 0, 1, 4},		12, {0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	betterCases[226]	= {/*todo, tyhmä lärppä*/};
	betterCases[227]	= {7, {2, 1, 19, 18, 4, 20, 21},			9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 227
	betterCases[228]	= {/*todo, tyhmä lärppä*/};
	betterCases[229]	= {7, {0, 3, 17, 19, 4, 20, 21},			9, {0,1,2, 2,1,3, 4,5,5}};				// Todo(Leo): suspicious half pipe thing 229
	betterCases[230]	= {7, {13, 12, 21, 20, 2, 3, 19},			9, {0,1,2, 2,1,3, 4,5,6}};				// Todo(Leo): suspicious half pipe thing 230
	betterCases[231]	= {6, {2, 3, 19, 4, 20, 21},				6, {0,1,2, 2,1,3}};
	betterCases[232]	= {6, {15, 14, 18, 17, 21, 20},				12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	betterCases[233]	= {9, {0, 14, 15, 1, 18, 15, 16, 20, 21},	9, {0,1,2, 3,4,5, 6,7,8}};
	betterCases[234]	= {5, {21, 18, 20, 15, 12},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[235]	= {6, {1, 18, 15, 4, 20, 21},				6, {0,1,2, 3,4,5}};
	betterCases[236]	= {5, {17, 20, 14, 21, 13},					9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[237]	= {6, {0, 14, 15, 4, 20, 21},				6, {0,1,2, 3,4,5}};
	betterCases[238]	= {4, {13, 12, 21, 20},						6, {0,1,2, 2,1,3}};
	betterCases[239]	= {3, {4, 20, 21},							3, {0,1,2}};

	/// 128 + 64 + 32 + 16
	betterCases[240]	= {4, {17, 16, 19, 18}, 		6, {0,1,2, 2,1,3}};
	betterCases[241]	= {5, {1, 0, 18, 17, 19},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[242]	= {5, {2, 12, 19, 16, 18},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[243]	= {4, {2, 1, 19, 18},			6, {0,1,2, 2,1,3}};
	betterCases[244]	= {5, {13, 3, 16, 19, 17},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[245]	= {4, {0, 3, 17, 19},			6, {0,1,2, 2,1,3}};
	betterCases[246]	= {6, {12, 16, 13, 2, 3, 19}, 	6, {0,1,2, 3,4,5}};
	betterCases[247]	= {3, {2, 3, 19},				3, {0,1,2}};
	betterCases[248]	= {5, {15, 14, 18, 17, 16},		9, {0,1,2, 2,1,3, 2,3,4}};
	betterCases[249]	= {6, {0, 14, 17, 1, 18, 15},	6, {0,1,2, 3,4,5}};
	betterCases[250]	= {4, {15, 12, 18, 16},			6, {0,1,2, 2,1,3}};
	betterCases[251]	= {3, {1, 18, 15},				3, {0,1,2}};
	betterCases[252]	= {4, {13, 14, 16, 17},			6, {0,1,2, 2,1,3}};
	betterCases[253]	= {3, {0, 14, 17},				3, {0,1,2}};
	betterCases[254]	= {3, {12, 16, 13},				3, {0,1,2}};
	betterCases[255]	= {};


	for (f32 z = -1; z < fieldResolution.z; z += scale)
	{
		for (f32 y = -1; y < fieldResolution.y; y += scale)
		{
			for (f32 x = -1; x < fieldResolution.x; x += scale)
			{
				f32 X = x + scale, Y = y + scale, Z = z + scale;

				f32 f001 = sample_field({x, y, z});
				f32 f002 = sample_field({X, y, z});
				f32 f004 = sample_field({x, Y, z});
				f32 f008 = sample_field({X, Y, z});
				f32 f016 = sample_field({x, y, Z});
				f32 f032 = sample_field({X, y, Z});
				f32 f064 = sample_field({x, Y, Z});
				f32 f128 = sample_field({X, Y, Z});

				u8 caseId 	= 1 * (f001 < 0);
				caseId 		+= 2 * (f002 < 0);
				caseId 		+= 4 * (f004 < 0);
				caseId 		+= 8 * (f008 < 0);
				caseId 		+= 16 * (f016 < 0);
				caseId 		+= 32 * (f032 < 0);
				caseId 		+= 64 * (f064 < 0);
				caseId 		+= 128 * (f128 < 0);

				f32 h = -0.5;
				f32 H = 0.5;

				v3 v001 = {h,h,h};
				v3 v002 = {H,h,h};
				v3 v004 = {h,H,h};
				v3 v008 = {H,H,h};
				v3 v016 = {h,h,H};
				v3 v032 = {H,h,H};
				v3 v064 = {h,H,H};
				v3 v128 = {H,H,H};

				v3 gridVertices [] 	= {v001, v002, v004, v008, v016, v032, v064, v128};
				f32 gridValues [] 	= {f001, f002, f004, f008, f016, f032, f064, f128};

				auto find_isosurface_position = [](v3 a, v3 b, f32 aValue, f32 bValue) -> v3
				{
					// todo(Leo): test alternative interpolation
					// v3 e02 = v002 + t02 * (v008 - v002)

					// Todo(Leo): maybe clamp, it's maybe slower, but also maybe better
					f32 t 		= -aValue / (bValue - aValue);
					// t 			= 0.5f;
					v3 result 	= interpolate_v3(a, b, t);
					return result;
				};

				{
					BetterEdgeStruct & c = betterCases[caseId];
					for (s32 i = 0; i < c.vertexCount; ++i)
					{
						s32 edgeId = c.edgeIdIds[i];

						s32 e0 = edges[edgeId][0];
						s32 e1 = edges[edgeId][1];

					 	v3 position = find_isosurface_position(gridVertices[e0], gridVertices[e1], gridValues[e0], gridValues[e1]);
						vertices[vertexCount + i].position = v3{x,y,z} + scale * position;
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
	}




	logDebug(0) << "Marching Cubes Took: " << platformApi->elapsed_seconds(startTime, platformApi->current_time()) << " s";
	logDebug(0) << "Vertices: " << vertexCount << "/" << vertexCapacity << ", indices: " << indexCount << "/" << indexCapacity;

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