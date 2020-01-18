/*=============================================================================
Leo Tamminen

Map Generator
=============================================================================*/

struct HexMap
{
	int 				cellCountPerDirection;
	real32 				cellSize;
	ArenaArray<u32> 	cells;

	real32 
	WorldSize() { return cellCountPerDirection * cellSize; }
 	
 	u32 & 
 	Cell (int x, int y) { return cells[x + y * cellCountPerDirection]; }

	vector3
	GetCellPosition(s32 x, s32 y)
	{
		// Note(Leo): math stolen from catlike coding hex tutorial
		real32 cellOuterRadius = cellSize;
		real32 cellInnerRadius = 0.866025404f * cellOuterRadius;
	
		real32 cellOffsetX = ((cellCountPerDirection * cellInnerRadius) * 2.0f) / 2.0f;
		real32 cellOffsetY = ((cellCountPerDirection * cellOuterRadius) * 1.5f) / 2.0f;

		vector3 cellPosition =
		{
			(x + y * 0.5f - (y / 2)) * cellInnerRadius * 2.0f - cellOffsetX,
			y * cellOuterRadius * 1.5f - cellOffsetY,
			0 
		};	
		return cellPosition;
	}
};


namespace map
{
	struct CreateInfo
	{
		s32 cellCountPerDirection;
		real32 cellSize;
	};

	internal HexMap
	GenerateMap(MemoryArena & memoryArena, CreateInfo & info);

	internal MeshAsset
	GenerateMapMesh (MemoryArena & memoryArena, HexMap & map);
}



void
AddHexCell(vector3 cellPosition, HexMap * map, bool blocked, u16 * vertexIndex, Vertex * vertexArray, u16 * triangleIndex, u16 * triangleArray)
{
	Vertex * vertexLocation = vertexArray + *vertexIndex;
	u16 * triangleLocation = triangleArray + *triangleIndex;

	vector3 color = blocked ? vector3{0.1f, 0.1f, 0.1f} : vector3{1, 1, 1};
	vector3 normal = {0, 0, 1};

	real32 padding = 0.02f;
	
	// Note(Leo): math stolen from catlike coding hex tutorial
	real32 cellTileOuterRadius = map->cellSize - 2.0f * padding;
	real32 cellTileInnerRadius = 0.866025404f * cellTileOuterRadius;

	vector3 hexCellCorners [6] =
	{
		vector3{0, cellTileOuterRadius, 0},
		vector3{-cellTileInnerRadius, 0.5f * cellTileOuterRadius, 0},
		vector3{-cellTileInnerRadius, -0.5f * cellTileOuterRadius, 0},
		vector3{0, -cellTileOuterRadius, 0},
		vector3{cellTileInnerRadius, -0.5f * cellTileOuterRadius, 0},
		vector3{cellTileInnerRadius, 0.5f * cellTileOuterRadius, 0},
	};

	vertexLocation[0].position = cellPosition;
	vertexLocation[1].position = cellPosition + hexCellCorners[0];
	vertexLocation[2].position = cellPosition + hexCellCorners[1];
	vertexLocation[3].position = cellPosition + hexCellCorners[2];
	vertexLocation[4].position = cellPosition + hexCellCorners[3];
	vertexLocation[5].position = cellPosition + hexCellCorners[4];
	vertexLocation[6].position = cellPosition + hexCellCorners[5];

	vertexLocation[0].color = color;
	vertexLocation[1].color = color;
	vertexLocation[2].color = color;
	vertexLocation[3].color = color;
	vertexLocation[4].color = color;
	vertexLocation[5].color = color;
	vertexLocation[6].color = color;

	vertexLocation[0].normal = normal;
	vertexLocation[1].normal = normal;
	vertexLocation[2].normal = normal;
	vertexLocation[3].normal = normal;
	vertexLocation[4].normal = normal;
	vertexLocation[5].normal = normal;
	vertexLocation[6].normal = normal;

	real32 gridSize = map->WorldSize();
	vertexLocation[0].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[0].position) / gridSize, 0.5f);
	vertexLocation[1].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[1].position) / gridSize, 0.5f);
	vertexLocation[2].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[2].position) / gridSize, 0.5f);
	vertexLocation[3].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[3].position) / gridSize, 0.5f);
	vertexLocation[4].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[4].position) / gridSize, 0.5f);
	vertexLocation[5].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[5].position) / gridSize, 0.5f);
	vertexLocation[6].texCoord = vector::coeff_add(size_cast<vector2>(vertexLocation[6].position) / gridSize, 0.5f);

	triangleLocation[0] = *vertexIndex;
	triangleLocation[1] = *vertexIndex + 1; 
	triangleLocation[2] = *vertexIndex + 2;
	
	triangleLocation[3] = *vertexIndex;
	triangleLocation[4] = *vertexIndex + 2;
	triangleLocation[5] = *vertexIndex + 3;

	triangleLocation[6] = *vertexIndex;
	triangleLocation[7] = *vertexIndex + 3;
	triangleLocation[8] = *vertexIndex + 4;

	triangleLocation[9] = *vertexIndex;
	triangleLocation[10] = *vertexIndex + 4;
	triangleLocation[11] = *vertexIndex + 5;

	triangleLocation[12] = *vertexIndex;
	triangleLocation[13] = *vertexIndex + 5;
	triangleLocation[14] = *vertexIndex + 6;

	triangleLocation[15] = *vertexIndex;
	triangleLocation[16] = *vertexIndex + 6;
	triangleLocation[17] = *vertexIndex + 1;

  	*vertexIndex += 7;
  	*triangleIndex += 18;
}

internal HexMap
map::GenerateMap(MemoryArena & memoryArena, map::CreateInfo & info)
{
	HexMap map = {};

	map.cellCountPerDirection = info.cellCountPerDirection;;
	map.cellSize = info.cellSize;

	map.cells = push_array<u32>(&memoryArena, map.cellCountPerDirection * map.cellCountPerDirection);

	for (s32 y = 0; y < map.cellCountPerDirection; ++y)
	{
		for (s32 x = 0; x < map.cellCountPerDirection; ++x)
		{
			map.Cell(x, y) = RandomValue() < 0.5f ? 0 : 1;
		}
	}
	return map;
}

internal MeshAsset
map::GenerateMapMesh(MemoryArena & memoryArena, HexMap & map)
{
	MeshAsset result = {};
	result.indexType = IndexType::UInt16;

	s32 vCount = map.cellCountPerDirection * map.cellCountPerDirection * 7;
	s32 iCount = map.cellCountPerDirection * map.cellCountPerDirection * 6 * 3;

	result.vertices = push_array<Vertex>(&memoryArena, vCount);
	result.indices 	= push_array<u16>(&memoryArena, iCount);

	u16 vertexIndex = 0;
	u16 triangleIndex = 0;

	for (s32 y = 0; y < map.cellCountPerDirection; ++y)
	{
		for (s32 x = 0; x < map.cellCountPerDirection; ++x)
		{

			// DANGER(Leo): array may have not been designed this in mind
			Vertex * vertexLocation = result.vertices.begin() + vertexIndex;
			u16 * indexLocation = result.indices.begin() + triangleIndex;

			// Todo(Leo): Lol clean :P
			AddHexCell(	map.GetCellPosition(x,y),
						&map,
						map.Cell(x, y) == 0,
						&vertexIndex, result.vertices.begin(),
						&triangleIndex, result.indices.begin());

		}
	}


	// Todo(Leo): We should trim our arena array down to actual size
	return result;

}