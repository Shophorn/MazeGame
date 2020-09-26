/*=============================================================================
Leo Tamminen 
shophorn @ internet

Skybox mesh creation and drawing.
=============================================================================*/

internal MeshAssetData
create_skybox_mesh(MemoryArena * allocator)
{
	Vertex vertices [] = 
	{
		// Vertex { .position = {-1, -1, -1}, },
		// Vertex { .position = { 1, -1, -1}, },
		// Vertex { .position = {-1,  1, -1}, },
		// Vertex { .position = { 1,  1, -1}, },
		// Vertex { .position = {-1, -1,  1}, },
		// Vertex { .position = { 1, -1,  1}, },
		// Vertex { .position = {-1,  1,  1}, },
		// Vertex { .position = { 1,  1,  1}, },

		// Down
		Vertex { .position = {-1, -1, -1}, .texCoord = { 0, 0}, },
		Vertex { .position = { 1, -1, -1}, .texCoord = { 1, 0}, },
		Vertex { .position = {-1,  1, -1}, .texCoord = { 0, 1}, },
		Vertex { .position = { 1,  1, -1}, .texCoord = { 1, 1}, },

		// Back
		Vertex { .position = {-1, -1, -1}, .texCoord = { 0, 0}, },
		Vertex { .position = { 1, -1, -1}, .texCoord = { 1, 0}, },
		Vertex { .position = {-1, -1,  1}, .texCoord = { 0, 1}, },
		Vertex { .position = { 1, -1,  1}, .texCoord = { 1, 1}, },

		// Left
		Vertex { .position = {-1, -1, -1}, .texCoord = { 0, 0}, },
		Vertex { .position = {-1,  1, -1}, .texCoord = { 1, 0}, },
		Vertex { .position = {-1, -1,  1}, .texCoord = { 0, 1}, },
		Vertex { .position = {-1,  1,  1}, .texCoord = { 1, 1}, },

		// Up
		Vertex { .position = {-1, -1,  1}, .texCoord = { 0, 0}, },
		Vertex { .position = { 1, -1,  1}, .texCoord = { 1, 0}, },
		Vertex { .position = {-1,  1,  1}, .texCoord = { 0, 1}, },
		Vertex { .position = { 1,  1,  1}, .texCoord = { 1, 1}, },

		// Forward
		Vertex { .position = { 1,  1, -1}, .texCoord = { 0, 0}, },
		Vertex { .position = {-1,  1, -1}, .texCoord = { 1, 0}, },
		Vertex { .position = { 1,  1,  1}, .texCoord = { 0, 1}, },
		Vertex { .position = {-1,  1,  1}, .texCoord = { 1, 1}, },

		// Right
		Vertex { .position = { 1, -1, -1}, .texCoord = { 0, 0}, },
		Vertex { .position = { 1,  1, -1}, .texCoord = { 1, 0}, },
		Vertex { .position = { 1, -1,  1}, .texCoord = { 0, 1}, },
		Vertex { .position = { 1,  1,  1}, .texCoord = { 1, 1}, },
	};

	// Face normals inside
	u16 indices [] =
	{
		0, 1, 2, 1, 3, 2,
		4, 6, 5, 6, 7, 5,
		9, 10, 8, 9, 11, 10,

		12, 14, 13, 14, 15, 13,
		16, 19, 17, 19, 16, 18,
		20, 22, 21, 22, 23, 21
	};

	MeshAssetData result = {};

	result.vertexCount 	= array_count(vertices);
	result.vertices 	= push_and_copy_memory(*allocator, array_count(vertices), vertices, 0);

	result.indexCount 	= array_count(indices);
	result.indices 		= push_and_copy_memory(*allocator, array_count(indices), indices, 0);
	return result;
}