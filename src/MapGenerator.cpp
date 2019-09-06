/*=============================================================================
Leo Tamminen

Map Generator
=============================================================================*/

namespace map
{
	internal MeshAsset
	Generate (MemoryArena * memoryArena);
}

// Note (leo): These are only temporarily here
int32 cellCountPerDirection	= 128;
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

/* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
this directly where it is used */
internal MeshAsset
map::Generate(MemoryArena * memoryArena)
{
	MeshAsset result = {};
	result.indexType = IndexType::UInt16;

	result.vertices = PushArray<Vertex>(memoryArena, vertexCount);
	result.indices 	= PushArray<uint16>(memoryArena, indexCount);

	int32 halfCellCount = cellCountPerDirection / 2;
	int32 firstCell 	= -halfCellCount;
	int32 lastCell 		= halfCellCount;

	for (int32 y = firstCell; y < lastCell; ++y)
	{
		for (int32 x = firstCell; x < lastCell; ++x)
		{
			int32 tileIndex = (x + halfCellCount) + ((y + halfCellCount) * cellCountPerDirection);
			int32 vertexIndex = tileIndex * 4;
			int32 triangleIndex = tileIndex * 6;

			Vertex * vertexLocation = result.vertices.memory + vertexIndex;
			uint16 * indexLocation = result.indices.memory + triangleIndex;

			/// ----- VERTICES -----
			vertexLocation[0].position	= { x * cellSize + padding, 			y * cellSize + padding, 			0};
			vertexLocation[1].position	= { (x + 1) * cellSize - padding, 		y * cellSize + padding, 			0};
			vertexLocation[2].position	= { x * cellSize + padding, 			(y + 1) * cellSize - padding, 		0};
			vertexLocation[3].position 	= { (x + 1) * cellSize - padding, 		(y + 1) * cellSize - padding, 		0};

			bool32 isBlack = ((y % 2) + x) % 2; 
			Vector3 color = isBlack ? Vector3{0.25f, 0.25f, 0.25f} : Vector3{0.6f, 0.6f, 0.6f};

			vertexLocation[0].color = color;
			vertexLocation[1].color = color;
			vertexLocation[2].color = color;
			vertexLocation[3].color = color;

			Vector3 normal = CoordinateSystem::Up;
			vertexLocation[0].normal = normal;
			vertexLocation[1].normal = normal;
			vertexLocation[2].normal = normal;
			vertexLocation[3].normal = normal;

			vertexLocation[0].texCoord = size_cast<Vector2>(vertexLocation[0].position) / gridSize + 0.5f;
			vertexLocation[1].texCoord = size_cast<Vector2>(vertexLocation[1].position) / gridSize + 0.5f;
			vertexLocation[2].texCoord = size_cast<Vector2>(vertexLocation[2].position) / gridSize + 0.5f;
			vertexLocation[3].texCoord = size_cast<Vector2>(vertexLocation[3].position) / gridSize + 0.5f;


			/// ----- INDICES -----
			indexLocation[0] = vertexIndex;
			indexLocation[1] = vertexIndex + 1;
			indexLocation[2] = vertexIndex + 2;
			indexLocation[3] = vertexIndex + 2;
			indexLocation[4] = vertexIndex + 1;
			indexLocation[5] = vertexIndex + 3;
		}
	}

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

// 			Vector3 normal = CoordinateSystem::Up;
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