/*=============================================================================
Leo Tamminen 
shophorn @ internet

Skybox mesh creation and drawing.
=============================================================================*/

internal MeshAsset
create_skybox(MemoryArena * arena)
{
	auto vertices = push_array<Vertex>(arena, 
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
	});

	// Face normals outside
	// auto indices = push_array<uint16>(arena,
	// {
	// 	0, 2, 1, 1, 2, 3,
	// 	4, 5, 6, 6, 5, 7,
	// 	9, 8, 10, 9, 10, 11,

	// 	12, 13, 14, 14, 13, 15,
	// 	16, 17, 19, 19, 18, 16,
	// 	20, 21, 22, 22, 21, 23
	// });

	// Face normals inside
	auto indices = push_array<uint16>(arena,
	{
		0, 1, 2, 1, 3, 2,
		4, 6, 5, 6, 7, 5,
		9, 10, 8, 9, 11, 10,

		12, 14, 13, 14, 15, 13,
		16, 19, 17, 19, 16, 18,
		20, 22, 21, 22, 23, 21
	});

	auto result = make_mesh_asset(vertices, indices);
	return result;
}

internal void
draw_skybox(RenderedObjectHandle skybox, Camera * camera, game::RenderInfo * renderer)
{
	// Note(Leo): For now, copy camera transform. When we get a proper shader, these can be removed.
	Matrix44 matrix = make_transform_matrix(camera->position);
	// Matrix44 matrix = get_rotation_matrix(camera);
	renderer->draw(skybox, matrix);
}