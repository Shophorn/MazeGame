/*=============================================================================
Leo Tamminen
shophorn @ internet

Terrain generator prototype
=============================================================================*/

template<typename T>
struct Range
{
	T min;
	T max;
};

struct HeightMap
{
	ArenaArray<float> 	values;
	uint32 				gridSize;
	float				worldSize;
	Range<float>		heightRange;
};

internal float
get_height_at(HeightMap * map, Vector2 relativePosition)
{
	if ( 	(vector::coeff_greater_than_or_equal(relativePosition, 0.0f) == false)
			|| (vector::coeff_less_than(relativePosition, map->worldSize) == false))
	{

		std::cout << "[get_height_at()]: escape early\n";
		return 0;
	}

	uint32 x = floor_to<uint32>(relativePosition.x / map->worldSize * map->gridSize);
	uint32 y = floor_to<uint32>(relativePosition.y / map->worldSize * map->gridSize);

	uint64 valueIndex 	= x + map->gridSize * y;
	float value 		= map->values[valueIndex];

	return interpolate(map->heightRange.min, map->heightRange.max, value);
}

internal HeightMap
make_heightmap(MemoryArena * memory, TextureAsset * texture, uint32 gridSize, float worldSize, float minHeight, float maxHeight)
{
	uint64 pixelCount 				= gridSize * gridSize;
	ArenaArray<float> heightValues 	= push_array<float>(memory, pixelCount);

	float scale = 1.0f / gridSize;

	for (float y = 0; y < gridSize; ++y)
	{
		for (float x = 0; x < gridSize; ++x)
		{
			uint64 mapIndex 		= x + gridSize * y;
			Vector2 texcoord 		= {x * scale, y * scale};
			uint32 pixel			= get_closest_pixel(texture, texcoord);
			heightValues[mapIndex] 	= get_red(pixel);
		}
	}

	return {
		.values 	= heightValues,
		.gridSize 	= gridSize,
		.worldSize 	= worldSize,

		.heightRange.min = minHeight,
		.heightRange.max = maxHeight,
	};
}

internal MeshAsset
generate_terrain(	MemoryArena * memory, 
					float textureScale,
					HeightMap * heightMap)
{
	int32 vertexCountPerSide 	= heightMap->gridSize;
	int32 vertexCount 			= vertexCountPerSide * vertexCountPerSide;

	int32 quadCountPerSide 		= heightMap->gridSize - 1;
	int32 triangleIndexCount 	= 6 * quadCountPerSide * quadCountPerSide;

	ArenaArray<Vertex> vertices	= reserve_array<Vertex>(memory, vertexCount);
	ArenaArray<uint16> indices	= reserve_array<uint16>(memory, triangleIndexCount);

	float scale = heightMap->worldSize / heightMap->gridSize;
	textureScale = textureScale / heightMap->gridSize;

	for (int32 y = 0; y < vertexCountPerSide; ++y)
	{
		for (int32 x = 0; x < vertexCountPerSide; ++x)
		{
			Vertex vertex = 
			{
				.position 	= {x * scale, y * scale, 0},
				.normal 	= {0, 0, 1},
				.texCoord 	= {x * textureScale, y * textureScale},
			};
			vertex.position.z = get_height_at(heightMap, {vertex.position.x, vertex.position.y});

			push_one(vertices, vertex);
		}
	}

	for (int32 y = 0; y < quadCountPerSide; ++y)
	{
		for (int x = 0; x < quadCountPerSide; ++x)
		{
			uint16 a = x + vertexCountPerSide * y;

			uint16 b = a + 1;
			uint16 c = a + vertexCountPerSide;
			uint16 d = a + 1 + vertexCountPerSide;

			push_many(indices, {a, b, c, c, b, d});
		}
	}
	return make_mesh_asset(vertices, indices);
}