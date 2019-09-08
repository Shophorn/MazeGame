/*=============================================================================
Leo Tamminen

Map Generator
=============================================================================*/

namespace map
{
	internal MeshAsset
	Generate (MemoryArena * memoryArena);
}

// Todo (leo): These variables are only temporarily here as globals
int32 cellCountPerDirection	= 60;
real32 cellSize				= 1.0f;

// Move cells so that they are relative to map center
real32 centeringValue 		= cellSize * cellCountPerDirection / 2.0f;
real32 gridSize 			= cellCountPerDirection * cellSize;

int32 vertexCountPerDirection 	= cellCountPerDirection * 2;
int32 vertexCount 				= vertexCountPerDirection * vertexCountPerDirection;
int32 indexCount 				= cellCountPerDirection * cellCountPerDirection * 6;

real32 uvStep = 1.0f / (real32)(cellCountPerDirection + 1);

real32 tileSize = 0.8f;
real32 padding = (cellSize - tileSize) / 2.0f;

// Note(Leo): math stolen from catlike coding hex tutorial
real32 cellOuterRadius = 1.0f;
real32 cellInnerRadius = 0.866025404f * cellOuterRadius;

real32 cellTileOuterRadius = 0.8 * cellOuterRadius;
real32 cellTileInnerRadius = 0.866025404f * cellTileOuterRadius;

Vector3 hexCellCorners [6] =
{
	Vector3{0, cellTileOuterRadius, 0},
	Vector3{-cellTileInnerRadius, 0.5f * cellTileOuterRadius, 0},
	Vector3{-cellTileInnerRadius, -0.5f * cellTileOuterRadius, 0},
	Vector3{0, -cellTileOuterRadius, 0},
	Vector3{cellTileInnerRadius, -0.5f * cellTileOuterRadius, 0},
	Vector3{cellTileInnerRadius, 0.5f * cellTileOuterRadius, 0},
};

void
AddHexCell(Vector3 cellPosition, uint16 * vertexIndex, Vertex * vertexArray, uint16 * triangleIndex, uint16 * triangleArray)
{
	Vertex * vertexLocation = vertexArray + *vertexIndex;
	uint16 * triangleLocation = triangleArray + *triangleIndex;

	Vector3 color = {1, 1, 1};
	Vector3 normal = {0, 0, 1};

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

	vertexLocation[0].texCoord = size_cast<Vector2>(vertexLocation[0].position) / gridSize + 0.5f;
	vertexLocation[1].texCoord = size_cast<Vector2>(vertexLocation[1].position) / gridSize + 0.5f;
	vertexLocation[2].texCoord = size_cast<Vector2>(vertexLocation[2].position) / gridSize + 0.5f;
	vertexLocation[3].texCoord = size_cast<Vector2>(vertexLocation[3].position) / gridSize + 0.5f;
	vertexLocation[4].texCoord = size_cast<Vector2>(vertexLocation[4].position) / gridSize + 0.5f;
	vertexLocation[5].texCoord = size_cast<Vector2>(vertexLocation[5].position) / gridSize + 0.5f;
	vertexLocation[6].texCoord = size_cast<Vector2>(vertexLocation[6].position) / gridSize + 0.5f;

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

void
AddSquareCell(Vector3 cellPosition, uint16 * vertexIndex, Vertex * vertexArray, uint16 * triangleIndex, uint16 * triangleArray)
{
	real32 span = tileSize / 2.0f;

	Vertex * vertexLocation = vertexArray + *vertexIndex;
	uint16 * triangleLocation = triangleArray + *triangleIndex;

	/// ----- VERTICES -----
	Vector3 color = {1, 1, 1};
	Vector3 normal = {0, 0, 1};

	vertexLocation[0].position = { cellPosition.x - span, cellPosition.y - span, 0};
	vertexLocation[1].position = { cellPosition.x + span, cellPosition.y - span, 0};
	vertexLocation[2].position = { cellPosition.x - span, cellPosition.y + span, 0};
	vertexLocation[3].position = { cellPosition.x + span, cellPosition.y + span, 0};

	vertexLocation[0].color = color;
	vertexLocation[1].color = color;
	vertexLocation[2].color = color;
	vertexLocation[3].color = color;

	vertexLocation[0].normal = normal;
	vertexLocation[1].normal = normal;
	vertexLocation[2].normal = normal;
	vertexLocation[3].normal = normal;

	vertexLocation[0].texCoord = size_cast<Vector2>(vertexLocation[0].position) / gridSize + 0.5f;
	vertexLocation[1].texCoord = size_cast<Vector2>(vertexLocation[1].position) / gridSize + 0.5f;
	vertexLocation[2].texCoord = size_cast<Vector2>(vertexLocation[2].position) / gridSize + 0.5f;
	vertexLocation[3].texCoord = size_cast<Vector2>(vertexLocation[3].position) / gridSize + 0.5f;


	/// ----- INDICES -----
	triangleLocation[0] = *vertexIndex;
	triangleLocation[1] = *vertexIndex + 1;
	triangleLocation[2] = *vertexIndex + 2;
	triangleLocation[3] = *vertexIndex + 2;
	triangleLocation[4] = *vertexIndex + 1;
	triangleLocation[5] = *vertexIndex + 3;



	*vertexIndex += 4;
	*triangleIndex += 6;
}


/* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
this directly where it is used */
internal MeshAsset
map::Generate(MemoryArena * memoryArena)
{
	MeshAsset result = {};
	result.indexType = IndexType::UInt16;

	int32 vCount = cellCountPerDirection * cellCountPerDirection * 7;
	int32 iCount = cellCountPerDirection * cellCountPerDirection * 6 * 3;

	result.vertices = PushArray<Vertex>(memoryArena, vCount);
	result.indices 	= PushArray<uint16>(memoryArena, iCount);

	int32 halfCellCount = cellCountPerDirection / 2;
	int32 firstCell 	= -halfCellCount;
	int32 lastCell 		= halfCellCount;

	uint16 vertexIndex = 0;
	uint16 triangleIndex = 0;

	for (int32 y = firstCell; y < lastCell; ++y)
	{
		for (int32 x = firstCell; x < lastCell; ++x)
		{
			Vertex * vertexLocation = result.vertices.memory + vertexIndex;
			uint16 * indexLocation = result.indices.memory + triangleIndex;

			// Note(leo): old, square grid positioning
			// Vector3 cellPosition = {(x + 0.5f) * cellSize, (y + 0.5f) * cellSize, 0};

			// Note(Leo): new, hex grid position
			Vector3 cellPosition =
			{
				(x + y * 0.5f - (y / 2)) * cellInnerRadius * 2.0f,
				y * cellOuterRadius * 1.5f,
				0 
			};
			AddHexCell(cellPosition, &vertexIndex, result.vertices.memory, &triangleIndex, result.indices.memory);

		}
	}


	// Todo(Leo): We should trim our arena array down to actual size
	return result;

}


// /*=============================================================================
// Leo Tamminen

// Map Generator
// =============================================================================*/

// namespace map
// {
// 	internal MeshAsset
// 	Generate (MemoryArena * memoryArena);
// }

// // Note (leo): These are only temporarily here
// int32 cellCountPerDirection	= 128;
// real32 cellSize				= 1.0f;

// // Move cells so that they are relative to map center
// real32 centeringValue 		= cellSize * cellCountPerDirection / 2.0f;
// real32 gridSize 			= cellCountPerDirection * cellSize;

// int32 vertexCountPerDirection 	= cellCountPerDirection * 2;
// int32 vertexCount 				= vertexCountPerDirection * vertexCountPerDirection;
// int32 indexCount 				= cellCountPerDirection * cellCountPerDirection * 6;

// real32 uvStep = 1.0f / (real32)(cellCountPerDirection + 1);


// real32 tileSize = 0.8f;
// real32 padding = (cellSize - tileSize) / 2.0f;

// /* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
// this directly where it is used */
// internal MeshAsset
// map::Generate(MemoryArena * memoryArena)
// {
// 	MeshAsset result = {};
// 	result.indexType = IndexType::UInt16;

// 	result.vertices = PushArray<Vertex>(memoryArena, vertexCount);
// 	result.indices 	= PushArray<uint16>(memoryArena, indexCount);

// 	for (int32 y = 0; y < cellCountPerDirection; ++y)
// 	{
// 		for (int32 x = 0; x < cellCountPerDirection; ++x)
// 		{
// 			int32 tileIndex = x + y * cellCountPerDirection;
// 			int32 vertexIndex = tileIndex * 4;
// 			int32 triangleIndex = tileIndex * 6;

// 			Vertex * vertexLocation = result.vertices.memory + vertexIndex;
// 			uint16 * indexLocation = result.indices.memory + triangleIndex;

// 			/// ----- VERTICES -----
// 			vertexLocation[0].position	= { x * cellSize - centeringValue + padding, 			y * cellSize + padding - centeringValue, 			0};
// 			vertexLocation[1].position	= { (x + 1) * cellSize - centeringValue - padding, 		y * cellSize + padding - centeringValue, 			0};
// 			vertexLocation[2].position	= { x * cellSize - centeringValue + padding, 			(y + 1) * cellSize - padding - centeringValue, 		0};
// 			vertexLocation[3].position 	= { (x + 1) * cellSize - centeringValue - padding, 		(y + 1) * cellSize - padding - centeringValue, 		0};

// 			bool32 isBlack = ((y % 2) + x) % 2; 
// 			Vector3 color = isBlack ? Vector3{0.25f, 0.25f, 0.25f} : Vector3{0.6f, 0.6f, 0.6f};

// 			vertexLocation[0].color = color;
// 			vertexLocation[1].color = color;
// 			vertexLocation[2].color = color;
// 			vertexLocation[3].color = color;

// 			Vector3 normal = World::Up;
// 			vertexLocation[0].normal = normal;
// 			vertexLocation[1].normal = normal;
// 			vertexLocation[2].normal = normal;
// 			vertexLocation[3].normal = normal;

// 			vertexLocation[0].texCoord = size_cast<Vector2>(vertexLocation[0].position) / gridSize + 0.5f;
// 			vertexLocation[1].texCoord = size_cast<Vector2>(vertexLocation[1].position) / gridSize + 0.5f;
// 			vertexLocation[2].texCoord = size_cast<Vector2>(vertexLocation[2].position) / gridSize + 0.5f;
// 			vertexLocation[3].texCoord = size_cast<Vector2>(vertexLocation[3].position) / gridSize + 0.5f;


// 			/// ----- INDICES -----
// 			indexLocation[0] = vertexIndex;
// 			indexLocation[1] = vertexIndex + 1;
// 			indexLocation[2] = vertexIndex + 2;
// 			indexLocation[3] = vertexIndex + 2;
// 			indexLocation[4] = vertexIndex + 1;
// 			indexLocation[5] = vertexIndex + 3;
// 		}
// 	}

// 	return result;

// }