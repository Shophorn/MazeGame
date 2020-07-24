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
v3 pos1 = {3,2,2};
v3 pos2 = {4,3,4};

internal u8 sample_field (f32 x, f32 y,  f32 z)
{
	f32 r = 1.5;

	f32 d1 = magnitude_v3(v3{x,y,z} - pos1);
	f32 d2 = magnitude_v3(v3{x,y,z} - pos2);

	f32 d = min_f32(d1, d2);


	// float distance = magnitude_v3(v3{x,y,z} - (fieldResolution / 2));

	return d < r ? 1 : 0;



	// Note(Leo): invert these so 'image' above goes to right directions
	// Todo(Leo): We maybe shouldn't do this, because of reasons
	s32 _y = round_f32(fieldResolution.y - 1 - y);
	s32 _z = round_f32(fieldResolution.z - 1 - z);
	
	s32 _x = round_f32(x);

	if (_x < 0 || _x >= fieldResolution.x || _y < 0 || _y >= fieldResolution.y || _z < 0 || _z >= fieldResolution.z)
	{
		return 0;
	}

	s32 index = _x + _y  * fieldResolution.x + _z  * fieldResolution.x * fieldResolution.y;
	return field[index];
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

	struct MarchingCubesCase
	{
		s32 vertexCount;
		v3 	vertices [12];
		s32 indexCount;
		u16 indices[18];
	};

	MarchingCubesCase cases [256] = {};
	f32 h = -0.5;
	f32 H = 0.5;

	v3 v00 = {0,h,h};
	v3 v01 = {h,0,h};
	v3 v02 = {H,0,h};
	v3 v03 = {0,H,h};
	v3 v04 = {h,h,0};
	v3 v05 = {H,h,0};
	v3 v06 = {h,H,0};
	v3 v07 = {H,H,0};
	v3 v08 = {0,h,H};
	v3 v09 = {h,0,H};
	v3 v10 = {H,0,H};
	v3 v11 = {0,H,H};

	v3 center = {0,0,0};

	cases[0] 	= {};
	cases[1] 	= {3, {v00, v01, v04}, 					3,{0,1,2}};	
	cases[2] 	= {3, {v00, v05, v02}, 					3,{0,1,2}};	
	cases[3] 	= {4, {v02, v01, v05, v04 },			6, {0,1,2, 2,1,3}};
	cases[4] 	= {3, {v03, v06, v01}, 					3, {0,1,2}};
	cases[5] 	= {4, {v00, v03, v04, v06 }, 			6, {0,1,2, 2,1,3}};
	cases[6]	= {6, {v00, v05, v02, v01, v03, v06}, 	6, {0,1,2, 3,4,5}};
	cases[7] 	= {5, {v02, v03, v05, v06, v04}, 		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[8] 	= {3, {v03, v02, v07}, 					3, {0,1,2}};
	cases[9]	= {6, {v00, v01, v04, v02, v07, v03}, 	6, {0,1,2, 3,4,5}};
	cases[10]	= {4, { v03, v00, v07, v05 }, 			6, {0,1,2, 2,1,3}};
	cases[11] 	= {5, {v03, v01, v07, v04, v05}, 		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[12] 	= {4, { v01, v02, v06, v07 },			6, {0,1,2, 2,1,3}};
	cases[13] 	= {5, {v00, v02, v04, v07, v06}, 		9, {0,1,2, 2,1,3, 2,3,4}};	
	cases[14] 	= {5, {v01, v00, v06, v05, v07}, 		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[15] 	= {4, {v04, v05, v06, v07}, 			6, {0,1,2, 2,1,3}};
	
	cases[16] 	= {3, {v08, v04, v09}, 									3,{0,1,2}};
	cases[17] 	= {4, {v00, v01, v08, v09}, 							6, {0,1,2, 2,1,3}};
	cases[18] 	= {6, {v08, v04, v09, v00, v05, v02}, 					6, {0,1,2, 3,4,5}};
	cases[19] 	= {5, {v08, v05, v09, v02, v01}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[20] 	= {6, {v03, v06, v01, v04, v09, v08}, 					6, {0,1,2, 3,4,5}};
	cases[21] 	= {5, {v06, v09, v03, v08, v00}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[22] 	= {9, {v00, v05, v02, v01, v03, v06, v04, v09, v08}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[23]	= {6, {v02, v03, v05, v06, v08, v09}, 					12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[24]	= {6, {v02, v07, v03, v04, v09, v08}, 					6, {0,1,2, 3,4,5}};
	cases[25] 	= {7, {v00, v01, v08, v09, v02, v07, v03}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[26] 	= {7, {v03, v00, v07, v05, v04, v09, v08}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[27]	= {7, {center, v01, v09, v08, v05, v07, v03}, 			18, {0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[28] 	= {7, {v01, v02, v06, v07, v04, v09, v08}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[29] 	= {7, {center, v00, v02, v07, v06, v09, v08},			18, {0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[30]	= {8, {v01, v00, v06, v05, v07, v04, v09, v08}, 		12, {0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[31] 	= {5, {v09, v08, v06, v05, v07}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	
	cases[32] 	= {3, {v08, v10, v05}, 									3,{0,1,2}};
	cases[33] 	= {6, {v00, v01, v04, v08, v10, v05}, 					6, {0,1,2, 3,4,5}};
	cases[34] 	= {4, {v02, v00, v10, v08}, 							6, {0,1,2, 2,1,3}};
	cases[35] 	= {5, {v04, v08, v01, v10, v02}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[36]	= {6, {v01, v03, v06, v05, v08, v10},					6, {0,1,2, 3,4,5}};
	cases[37] 	= {7, {v00, v03, v04, v06, v05, v08, v10}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[38] 	= {7, {v02, v00, v10, v08, v01, v03, v06}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[39]	= {7, {center, v02, v03, v06, v04, v08, v10}, 			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[40] 	= {6, {v02, v07, v03, v05, v08, v10}, 					6, {0,1,2, 3,4,5}};
	cases[41] 	= {9, {v00, v01, v04, v02, v07, v03, v05, v08, v10}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[42] 	= {5, {v10, v07, v08, v03, v00}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[43]	= {6, {v03, v01, v07, v04, v10, v08}, 					12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[44] 	= {7, {v01, v02, v06, v07, v06, v11, v09}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[45]	= {8, {v00, v02, v04, v07, v06, v05, v08, v10}, 		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[46]	= {7, {center, v00, v08, v10, v07, v06, v01}, 			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[47] 	= {5, {v08, v10, v04, v07, v06}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	
	cases[48] 	= {4, { v05, v04, v10, v09 }, 					6, {0,1,2, 2,1,3}};
	cases[49] 	= {5, {v05, v00, v10, v01, v09}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[50] 	= {5, {v00, v04, v02, v09, v10}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[51] 	= {4, {v02, v01, v10, v09}, 					6, {0,1,2, 2,1,3}};
	cases[52] 	= {7, {v05, v04, v10, v09, v01, v03, v06}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[53]	= {7, {center, v00, v03, v06, v09, v10, v05}, 	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[54]	= {8, {v00, v04, v02, v09, v10, v01, v03, v06}, 12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[55]	= {5, {v03, v06, v02, v09, v10}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[56] 	= {7, {v05, v04, v10, v09, v02, v07, v03}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[57]	= {8, {v05, v00, v10, v01, v09, v02, v07, v03},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[58]	= {7, {center, v00, v04, v09, v10, v07, v03}, 	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[59]	= {5, {v07, v03, v10, v01, v09},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[60]	= {8, {v01,v02, v06, v07, v05, v04, v10, v09}, 	12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	cases[61]	= {7, {v07, v06, v10, v09, v00, v02, v05}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[62]	= {7, {v07, v06, v10, v09, v00, v04, v01}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[63]	= {4, {v07, v06, v10, v09}, 					6, {0,1,2, 2,1,3}};

	cases[64] 	= {3, {v11, v09, v06}, 									3, {0,1,2}};
	cases[65] 	= {6, {v00, v01, v04, v06, v11, v09}, 					6, {0,1,2, 3,4,5}};
	cases[66]	= {6, {v00, v05, v02, v06, v11, v09},					6, {0,1,2, 3,4,5}};
	cases[67] 	= {7, {v02, v01, v05, v04, v06, v11, v09}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[68] 	= {4, {v01, v03, v09, v11}, 							6, {0,1,2, 2,1,3}};
	cases[69] 	= {5, {v09, v04, v11, v00, v03}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[70] 	= {7, {v01, v03, v09, v11, v00, v05, v02}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[71]	= {7, {center, v02, v03, v11, v09, v04, v05}, 			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[72] 	= {6, {v02, v07, v03, v06, v11, v09}, 					6, {0,1,2, 3,4,5}};
	cases[73] 	= {9, {v00, v01, v04, v02, v07, v03, v06, v11, v09}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[74] 	= {7, {v03, v00, v07, v05, v06, v11, v09}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[75]	= {8, {v03, v01, v07, v04, v05, v06, v11, v09}, 		12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[76] 	= {5, {v07, v11, v02, v09, v01}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[77] 	= {6, {v00, v02, v04, v07, v09, v11}, 					12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[78] 	= {7, {center, v01, v00, v05, v07, v11, v09}, 			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[79] 	= {5, {v11, v09, v07, v04, v05}, 						9, {0,1,2, 2,1,3, 2,3,4}};

	cases[80] 	= {4, { v11, v08, v06, v04 }, 					6, {0,1,2, 2,1,3}};
	cases[81] 	= {5, {v01, v06, v00, v11, v08}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[82] 	= {7, {v04, v06, v08, v11, v00, v05, v02}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[83]	= {7, {center, v02, v01, v06, v11, v08, v05}, 	18, {0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[84] 	= {5, {v04, v01, v08, v03, v11}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[85] 	= {4, {v00, v03, v08, v11}, 					6, {0,1,2, 2,1,3}};
	cases[86]	= {8, {v04, v01, v08, v03, v11, v00, v05, v02},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[87]	= {5, {v05, v02, v08, v03, v11},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[88] 	= {7, {v04, v06, v08, v11, v02, v07, v03}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[89]	= {8, {v01, v06, v00, v11, v08, v02, v07, v03},	12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[90]	= {8, {v03, v00, v07, v05, v04, v06, v08, v11}, 12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	// Todo(Leo): this seems super weird
	cases[91]	= {7, {v05, v07, v08, v11, v01, v06, v03}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[92]	= {7, {center, v01, v02, v07, v11, v08, v04},	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[93]	= {5, {v02, v07, v00, v11, v08},				9, {0,1,2, 2,1,3, 2,3,4}};
	// Todo(Leo): this seems super weird
	cases[94]	= {7, {v05, v07, v08, v11, v00, v04, v01}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[95]	= {4, {v05, v07, v08, v11}, 					6, {0,1,2, 2,1,3}};
	
	cases[96]	= {6, {v05, v08, v10, v06, v11, v09}, 									6, {0,1,2, 3,4,5}};
	cases[97] 	= {9, {v00, v01, v04, v05, v08, v10, v06, v11, v09}, 					9, {0,1,2, 3,4,5, 6,7,8}};
	cases[98] 	= {7, {v02, v00, v10, v08, v06, v11, v09}, 								9, {0,1,2, 2,1,3, 4,5,6}};
	cases[99]	= {8, {v04, v08, v01, v10, v02, v06, v11, v09},							12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[100] 	= {7, {v01, v03, v09, v11, v05, v08, v10}, 								9, {0,1,2, 2,1,3, 4,5,6}};
	cases[101]	= {7, {v09, v04, v11, v00, v03, v05, v08, v10},							12,{0,1,2, 2,1,3, 4,5,6}};
	cases[102] 	= {8, {v02, v00, v10, v08, v01, v03, v09, v11}, 						12, {0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	// Todo(Leo): this seems super weird
	cases[103]	= {7, {v02, v03, v10, v11, v04, v08, v09},								9, {0,1,3, 2,1,3, 4,5,6}};
	cases[104] 	= {9, {v02, v07, v03, v05, v08, v10, v06, v11, v09}, 					9, {0,1,2, 3,4,5, 6,7,8}};
	cases[105]	= {12, {v00, v01, v04, v02, v07, v03, v05, v08, v10, v06, v11 ,v09}, 	12,{0,1,2, 3,4,5, 6,7,8, 9,10,11}};
	cases[106]	= {8, {v10, v02, v08, v03, v00, v06, v11, v09},							12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[107]	= {9, {v01, v06, v03, v04, v08, v09, v07, v11, v10},					9, {0,1,2, 3,4,5, 6,7,8}};
	cases[108]	= {8, {v07, v11,v02, v09, v01, v05, v08, v10}, 							12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[109]	= {9, {v00, v05, v02, v04, v08, v09, v07, v11, v10},					9, {0,1,2, 3,4,5, 6,7,8}};

	cases[110]	= {7, {v01, v00, v09, v08, v07, v11, v10},				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[111]	= {6, {v04, v06, v09, v07, v11, v10},					6, {0,1,2, 3,4,5}};
	cases[112] 	= {5, {v11, v10, v06, v05, v04}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[113]	= {6, {v00, v01, v05, v06, v10, v11},					12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[114]	= {7, {center, v02, v00, v04, v06, v11, v10},			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[115]	= {5, {v06, v11, v01, v10, v02},						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[116]	= {7, {center, v01, v03, v11, v10, v05, v04},			18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[117]	= {5, {v10, v05, v11, v00, v03},						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[118]	= {7, {v02, v03, v10, v11, v00, v04, v01},				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[119] 	= {4, {v02, v03, v10, v11}, 							6, {0,1,2, 2,1,3}};
	cases[120]	= {8, {v11, v10, v06, v05, v04, v02, v07, v03},			12,{0,1,2, 2,1,3, 2,3,4, 5,6,7}};
	cases[121]	= {9, {v00, v02, v05, v01, v06, v03, v07, v11, v10}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	// todo(LEo): weird...
	cases[122]	= {7, {v03, v00, v06, v04, v07, v11, v10},				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[123]	= {6, {v01, v06, v03, v07, v11, v10}, 					6, {0,1,2, 3,4,5}};
	// todo(Leo): weird case again
	cases[124]	= {7, {v01, v02, v04, v05, v07, v11, v10},				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[125]	= {6, {v00, v02, v05, v07, v11, v10},					6, {0,1,2, 3,4,5}};
	cases[126]	= {6, {v00, v04, v01, v07, v11, v10}, 					6, {0,1,2, 3,4,5}};
	cases[127]	= {3, {v07, v11, v10}, 									3, {0,1,2}};
	
	cases[128] 	= {3, {v11, v07, v10}, 3,{0,1,2}};	
	cases[129]	= {/*TODO*/};
	cases[130] 	= {6, {v00, v05, v02, v07, v10, v11}, 					6, {0,1,2, 3,4,5}};
	cases[131] 	= {7, {v02, v01, v05, v04, v07, v10, v11}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[132] 	= {6, {v01, v03, v06, v07, v10, v11}, 					6, {0,1,2, 3,4,5}};
	cases[133] 	= {7, {v00, v03, v04, v06, v07, v10, v11}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[134] 	= {9, {v00, v05, v02, v01, v03, v06, v07, v10, v11}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[135]	= {/*TODO*/};
	cases[136] 	= {4, {v03, v02, v11, v10}, 				6, {0,1,2, 2,1,3}};
	cases[137] 	= {7, {v03, v02, v11, v10, v00, v01, v04}, 	9, {0,1,2, 2,1,3, 4,5,6}};
	cases[138] 	= {5, {v05, v10, v00, v11, v03}, 			9, {0,1,2, 2,1,3, 2,3,4}};
	cases[139]	= {/*TODO*/};
	cases[140] 	= {5, {v11, v06, v10, v01, v02}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	cases[141]	= {/*TODO*/};
	cases[142]	= {6, {v01, v00, v06, v05, v11, v10}, 					12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[143] 	= {5, {v10, v11, v05, v06, v04}, 						9, {0,1,2, 2,1,3, 2,3,4}};
	
	cases[144]	= {6, {v04, v09, v08, v07, v10, v11}, 					6, {0,1,2, 3,4,5}};
	cases[145] 	= {7, {v00, v01, v08, v09, v07, v10, v11}, 				9, {0,1,2, 2,1,3, 4,5,6}};
	cases[146] 	= {9, {v00, v05, v02, v04, v09, v08, v07, v10, v11}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[147]	= {/*TODO*/};
	cases[148]	= {9, {v01, v03, v06, v04, v09, v08, v07, v10, v11}, 	9, {0,1,2, 3,4,5, 6,7,8}};
	cases[149]	= {/*TODO*/};	
	cases[150]	= {/*TODO*/};
	cases[151]	= {/*TODO*/};
	cases[152] 	= {7, {v03, v02, v11, v10, v04, v09, v08}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[153]	= {8, {v00, v01, v08, v09, v03, v02, v11, v10}, 12, {0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	cases[154]	= {/*TODO*/};
	cases[155]	= {/*TODO*/};
	cases[156]	= {/*TODO*/};
	cases[157]	= {/*TODO*/};
	cases[158]	= {/*TODO*/};
	cases[159]	= {/*TODO*/};

	cases[160]	= {4, { v08, v11, v05, v07 }, 					6, {0,1,2, 2,1,3}};
	cases[161] 	= {7, {v07, v05, v11, v08, v00, v01, v04}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[162] 	= {5, {v07, v02, v11, v00, v08}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[163]	= {7, {center, v01, v04, v08, v11, v07, v02},	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[164] 	= {7, {v07, v05, v11, v08, v01, v03, v06}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[165]	= {8, {v00, v03, v04, v06, v07, v05, v11, v08}, 12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	cases[166]	= {/*TODO*/};
	cases[167]	= {/*TODO*/};
	cases[168] 	= {5, {v02, v05, v03, v08, v11}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[169]	= {/*TODO*/};
	cases[170] 	= {4, {v03, v00, v11, v08}, 					6, {0,1,2, 2,1,3}};
	cases[171]	= {5, {v01, v04, v03, v08, v11},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[172]	= {/*TODO*/};
	cases[173]	= {/*TODO*/};
	cases[174]	= {5, {v06, v01, v11, v00, v08},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[175]	= {4, {v06, v04, v11, v08}, 					6, {0,1,2, 2,1,3}};
	
	cases[176] 	= {5, {v09, v11, v04, v07, v05}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[177]	= {7, {center, v00, v01, v09, v11, v07, v05}, 	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[178]	= {6, {v02, v00, v07, v04, v11, v09}, 			12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[179]	= {5, {v07, v03, v10, v01, v09},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[180]	= {/*TODO*/};
	cases[181]	= {/*TODO*/};
	cases[182]	= {/*TODO*/};
	cases[183]	= {3, {v02, v03, v07, v06, v09, v11},			6, {0,1,2, 3,4,5}};
	cases[184]	= {/*TODO*/};
	cases[185]	= {/*TODO*/};
	cases[186]	= {5, {v06, v09, v00, v11, v03},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[187] 	= {4, {v03, v01, v11, v09}, 					6, {0,1,2, 2,1,3}};
	cases[188]	= {/*TODO*/};
	cases[189]	= {/*TODO*/};
	cases[190]	= {/*TODO*/};
	cases[191]	= {3, {v06, v09, v11}, 							3, {0,1,2}};
	
	cases[192]	= {4, { v06, v07, v09, v10 }, 					6, {0,1,2, 2,1,3}};
	cases[193] 	= {7, {v06, v07, v09, v10, v00, v01, v04}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[194] 	= {7, {v06, v07, v09, v10, v00, v05, v02}, 		9, {0,1,2, 2,1,3, 4,5,6}};
	cases[195]	= {8, {v02, v01, v05, v04, v06, v07, v09, v10}, 12,{0,1,2, 2,1,3, 4,5,6, 6,5,7}};
	cases[196] 	= {5, {v03, v07, v01, v10, v09}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[197]	= {7, {center, v00, v03, v06, v09, v10, v05},	18,{0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[198]	= {/*TODO*/};
	cases[199]	= {/*TODO*/};
	cases[200] 	= {5, {v06, v03, v09, v02, v10}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[201]	= {/*TODO*/};
	cases[202]	= {/*TODO*/};
	cases[203]	= {/*TODO*/};
	cases[204] 	= {4, {v01, v02, v09, v10}, 					6, {0,1,2, 2,1,3}};
	cases[205]	= {5, {v00, v04, v02, v09, v10},				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[206]	= {5, {v05, v00, v10, v01, v09}, 				9, {0,1,2, 2,1,3, 2,3,4}};
	cases[207]	= {4, {v04, v05, v09, v10}, 					6, {0,1,2, 2,1,3}};
	
	cases[208] 	= {5, {v10, v08, v07, v04, v06}, 		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[209]	= {/*TODO*/};
	cases[210]	= {/*TODO*/};
	cases[211]	= {/*TODO*/};
	cases[212] 	= {6, {v01, v03, v04, v07, v08, v10}, 	12,{0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[213]	= {5, {v10, v07, v08, v03, v00},		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[214]	= {/*TODO*/};
	cases[215]	= {6, {v02, v03, v07, v05, v10, v08},	6, {0,1,2, 3,4,5}};
	cases[216]	= {/*TODO*/};
	cases[217]	= {/*TODO*/};
	cases[218]	= {/*TODO*/};
	cases[219]	= {/*TODO*/};
	cases[220]	= {5, {v04, v08, v01, v10, v02},		9, {0,1,2, 2,1,3, 2,3,4	}};
	cases[221] 	= {4, {v00, v02, v08, v10}, 			6, {0,1,2, 2,1,3}};
	cases[222]	= {/*TODO*/};
	cases[223]	= {3, {v05, v10, v08}, 					3, {0,1,2}};
	
	cases[224] 	= {5, {v08, v09, v05, v06, v07}, 9, {0,1,2, 2,1,3, 2,3,4}};
	cases[225]	= {/*TODO*/};
	cases[226]	= {/*TODO*/};
	cases[227]	= {/*TODO*/};
	cases[228]	= {7, {center, v01, v03, v07, v05, v08, v09}, 18, {0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,1}};
	cases[229]	= {/*TODO*/};
	cases[230]	= {/*TODO*/};
	cases[231]	= {/*TODO*/};
	cases[232]	= {6, {v03, v02, v06, v05, v09, v08}, 		12, {0,1,2, 2,1,3, 2,3,4, 4,3,5}};
	cases[233]	= {/*TODO*/};
	cases[234]	= {5, {v09, v06, v08, v03, v00}, 			9, {0,1,2, 2,1,3, 2,3,4}};
	cases[235]	= {6, {v04, v06, v09, v01, v06, v03},		6, {0,1,2, 3,4,5}};
	cases[236]	= {5, {v05, v08, v02, v09, v01}, 			9, {0,1,2, 2,1,3, 2,3,4}};
	cases[237]	= {6, {v00, v02, v05, v04, v06, v09},		6, {0,1,2, 3,4,5}};
	cases[238] 	= {4, {v01, v00, v09, v08}, 				6, {0,1,2, 2,1,3}};
	cases[239]	= {3, {v04, v06, v09}, 						3, {0,1,2}};

	cases[240]	= {4, {v05, v04, v07, v06}, 		6, {0,1,2, 2,1,3}};
	cases[241]	= {5, {v00, v01,v05, v06, v07},		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[242]	= {5, {v02, v00, v07, v04, v06},		9, {0,1,2, 2,1,3, 2,3,4}};
	cases[243]	= {4, {v02, v01, v07, v06}, 		6, {0,1,2, 2,1,3}};
	cases[244]	= {5, {v01, v03, v04, v07, v05}, 	9, {0,1,2, 2,1,3, 2,3,4}};
	cases[245]	= {4, {v00, v03, v05, v07}, 		6, {0,1,2, 2,1,3}};
	cases[246]	= {/*TODO*/};
	cases[247]	= {3, {v02, v03, v07}, 				3, {0,1,2}};
	cases[248]	= {5, {v03, v02, v06, v05, v04},	9, {0,1,2, 2,1,3, 2,3,4}};
	cases[249]	= {/*TODO*/};
	cases[250]	= {4, {v03, v00, v06, v04}, 		6, {0,1,2, 2,1,3}};
	cases[251]	= {3, {v01, v04, v03},				3, {0,1,2}};
	cases[252]	= {4, {v01, v02, v04, v05}, 		6, {0,1,2, 2,1,3}};
	cases[253]	= {3, {v00, v02, v05}, 				3, {0,1,2}};
	cases[254]	= {3, {v00, v04, v01}, 				3, {0,1,2}};
	cases[255]	= {};

	logDebug(0) << "sizeof(cases) = " << sizeof(cases);

	PlatformTimePoint startTime = platformApi->current_time();

	f32 scale = 0.1;

	for (f32 z = -1; z < fieldResolution.z; z += scale)
	{
		for (f32 y = -1; y < fieldResolution.y; y += scale)
		{
			for (f32 x = -1; x < fieldResolution.x; x += scale)
			{
				u8 caseId = 1 * sample_field(x, 		y, 			z);
				caseId += 2 * 	sample_field(x + scale, 	y, 			z);
				caseId += 4 * 	sample_field(x, 		y + scale, 	z);
				caseId += 8 * 	sample_field(x + scale, 	y + scale, 	z);
				caseId += 16 * 	sample_field(x, 		y, 			z + scale);
				caseId += 32 * 	sample_field(x + scale, 	y, 			z + scale);
				caseId += 64 * 	sample_field(x, 		y + scale, 	z + scale);
				caseId += 128 * sample_field(x + scale, 	y + scale, 	z + scale);

				// logDebug(0) << v3{(f32)x,(f32)y,(f32)z} << ", " << (u32)caseId;

				Assert(cases[caseId].vertexCount + vertexCount < vertexCapacity);
				Assert(cases[caseId].indexCount + indexCount < indexCapacity);

				if (cases[caseId].vertexCount == 0 && caseId != 0 && caseId != 255)
				{
					logDebug(0) << "Unimplemented case " << (u32)caseId;
				}

				// logDebug(0) << "(" << x << ", " << y << ", " << z << "):" << (u32)caseId;	

				for (s32 i = 0; i < cases[caseId].vertexCount; ++i)
				{
					vertices[vertexCount + i].position = v3{x, y, z} + scale * cases[caseId].vertices[i];
					// Todo(Leo): not right obviously, but helps seeing generated shapes for now
					vertices[vertexCount + i].texCoord = cases[caseId].vertices[i].xy;
				}

				for (s32 i = 0; i < cases[caseId].indexCount; ++i)
				{
					indices[indexCount + i] = vertexCount + cases[caseId].indices[i];
				}

				vertexCount += cases[caseId].vertexCount;
				indexCount += cases[caseId].indexCount;
			}
		}
	}


	#if 0
	for (s32 y = 0; y < 16; ++y)
	{
		for (s32 x = 0; x < 16; ++x)
		{
			v3 position = v3{-30.0f + x, -30.0f + y, 0};
			s32 caseId = 16 * y + x;

			for (s32 i = 0; i < cases[caseId].vertexCount; ++i)
			{
				vertices[vertexCount + i].position = position + cases[caseId].vertices[i];
				// Todo(Leo): not right obviously, but helps seeing generated shapes for now
				vertices[vertexCount + i].texCoord = cases[caseId].vertices[i].xy;
			}

			for (s32 i = 0; i < cases[caseId].indexCount; ++i)
			{
				indices[indexCount + i] = vertexCount + cases[caseId].indices[i];
			}

			vertexCount += cases[caseId].vertexCount;
			indexCount += cases[caseId].indexCount;

		}
	}
	#endif



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