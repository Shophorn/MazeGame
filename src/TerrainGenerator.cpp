/*=============================================================================
Leo Tamminen
shophorn @ internet

Terrain generator prototype
=============================================================================*/

struct HeightMap
{
	ArenaArray<float> 	values;
	u32 				gridSize;
	float				worldSize;
	float 				minHeight;
	float				maxHeight;
};

internal float
get_height_at(HeightMap * map, vector2 relativePosition)
{
	vector2 gridSpacePosition = relativePosition * (map->gridSize / map->worldSize);

	s32 x0 		= floor_to<s32>(gridSpacePosition.x);
	s32 x1 		= x0 + 1;
	float xFraction = gridSpacePosition.x - x0;

	s32 y0 		= floor_to<s32>(gridSpacePosition.y);
	s32 y1 		= y0 + 1;
	float yFraction = gridSpacePosition.y - y0;

	if ((x0 < 0) || (x1 >= map->gridSize) || (y0 < 0) || (y1 >= map->gridSize))
	{
		return 0;
	}

	auto get_value = [map](s32 x, s32 y) -> float
	{
		u64 valueIndex = x + y * map->gridSize;
		return map->values[valueIndex];
	};

	float value00 	= get_value(x0, y0);
	float value10 	= get_value(x1, y0);
	float value01 	= get_value(x0, y1);
	float value11 	= get_value(x1, y1);

	float value0 	= interpolate(value00, value10, xFraction);
	float value1 	= interpolate(value01, value11, xFraction);

	float value 	= interpolate(value0, value1, yFraction);

	return interpolate(map->minHeight, map->maxHeight, value);
}

internal HeightMap
make_heightmap(MemoryArena * memory, TextureAsset * texture, u32 gridSize, float worldSize, float minHeight, float maxHeight)
{
	u64 pixelCount 				= gridSize * gridSize;
	ArenaArray<float> heightValues 	= push_array<float>(memory, pixelCount);

	float textureScale = 1.0f / gridSize;

	auto get_value = [texture](u32 u, u32 v) -> float
	{
		auto pixel 	= get_pixel(texture, u, v);
		float red 	= get_red(pixel);
		return red;
	};

	for (float y = 0; y < gridSize; ++y)
	{
		for (float x = 0; x < gridSize; ++x)
		{
			vector2 pixelCoord 	= {	x * textureScale * (texture->width - 1),
									y * textureScale * (texture->height - 1)};

			u32 u0 		= floor_to<u32>(pixelCoord.x);
			u32 u1 		= u0 + 1;
			float uFraction = pixelCoord.x - u0;
			
			u32 v0 		= floor_to<u32>(pixelCoord.y);
			u32 v1 		= v0 + 1;
			float vFraction	= pixelCoord.y - v0;

			float height00  = get_value(u0, v0);
			float height10  = get_value(u1, v0);
			float height01  = get_value(u0, v1);
			float height11  = get_value(u1, v1);

			float height0 	= interpolate(height00, height10, uFraction);
			float height1 	= interpolate(height01, height11, uFraction);

			float height 	= interpolate (height0, height1, vFraction);

			u64 mapIndex 		= x + gridSize * y;
			heightValues[mapIndex] 	= height;
		}
	}

	return {
		.values 	= heightValues,
		.gridSize 	= gridSize,
		.worldSize 	= worldSize,

		.minHeight = minHeight,
		.maxHeight = maxHeight,
	};
}

internal MeshAsset
generate_terrain(	MemoryArena * memory, 
					float texcoordScale,
					HeightMap * heightMap)
{
	s32 vertexCountPerSide 	= heightMap->gridSize;
	s32 vertexCount 			= vertexCountPerSide * vertexCountPerSide;

	s32 quadCountPerSide 		= heightMap->gridSize - 1;
	s32 triangleIndexCount 	= 6 * quadCountPerSide * quadCountPerSide;

	BETTERArray<Vertex> vertices	= allocate_BETTER_array<Vertex>(*memory, vertexCount);
	BETTERArray<u16> indices	= allocate_BETTER_array<u16>(*memory, triangleIndexCount);

	float scale = heightMap->worldSize / heightMap->gridSize;
	texcoordScale = texcoordScale / heightMap->gridSize;

	auto get_clamped_height = [heightMap](u32 x, u32 y) -> float
	{
		x = clamp(x, 0u, heightMap->gridSize - 1);
		y = clamp(y, 0u, heightMap->gridSize - 1);

		u32 valueIndex = x + y * heightMap->gridSize;
		float result = interpolate(	heightMap->minHeight,
									heightMap->maxHeight,
									heightMap->values[valueIndex]);
		return result;
	};

	float gridTileSize = heightMap->worldSize / heightMap->gridSize;

	for (s32 y = 0; y < vertexCountPerSide; ++y)
	{
		for (s32 x = 0; x < vertexCountPerSide; ++x)
		{
			/* Note(Leo): for normals, use "Finite Differece Method".
		 	https://stackoverflow.com/questions/13983189/opengl-how-to-calculate-normals-in-a-terrain-height-grid

			Study(Leo): This is supposed to be more accurate. Also look wikipedia.
			https://stackoverflow.com/questions/6656358/calculating-normals-in-a-triangle-mesh/21660173#21660173
			*/
			float height 		= get_clamped_height(x, y);
			float heightLeft 	= get_clamped_height(x - 1, y);			
			float heightRight 	= get_clamped_height(x + 1, y);			
			float heightBack 	= get_clamped_height(x, y - 1);			
			float heightFront 	= get_clamped_height(x, y + 1);			

			v3 normal = {	heightLeft - heightRight,
								heightBack - heightFront,
								2 * gridTileSize};
			normal = vector::normalize(normal);

			Vertex vertex = 
			{
				.position 	= {x * scale, y * scale, height},
				.normal 	= normal,
				.texCoord 	= {x * texcoordScale, y * texcoordScale},
			};
			vertices.push(vertex);
		}
	}

	for (s32 y = 0; y < quadCountPerSide; ++y)
	{
		for (int x = 0; x < quadCountPerSide; ++x)
		{
			u16 a = x + vertexCountPerSide * y;

			u16 b = a + 1;
			u16 c = a + vertexCountPerSide;
			u16 d = a + 1 + vertexCountPerSide;

			indices.push_many({a, b, c, c, b, d});
		}
	}
	return make_mesh_asset(std::move(vertices), std::move(indices));
}