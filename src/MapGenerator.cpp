/*=============================================================================
Leo Tamminen

Map Generator
=============================================================================*/

/* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
this directly where it is used */
internal Mesh
GenerateMap(MemoryArena * memoryArena)
{
	Mesh result = {};
	result.indexType = IndexType::UInt16;

	int32 tileCountPerDirection	= 128;
	real32 tileSize				= 1.0f;
	real32 centeringValue 		= tileSize * tileCountPerDirection / 2.0f;
	
	int32 vertexCountPerDirection 	= tileCountPerDirection * 2;
	int32 vertexCount 				= vertexCountPerDirection * vertexCountPerDirection;
	result.vertices = PushArray<Vertex>(memoryArena, vertexCount);

	int32 indexCount = tileCountPerDirection * tileCountPerDirection * 6;
	result.indices = PushArray<uint16>(memoryArena, indexCount);

	real32 uvStep = 1.0f / (real32)(tileCountPerDirection + 1);

	for (int32 z = 0; z < tileCountPerDirection; ++z)
	{
		for (int32 x = 0; x < tileCountPerDirection; ++x)
		{
			int32 tileIndex = x + z * tileCountPerDirection;
			int32 vertexIndex = tileIndex * 4;
			int32 triangleIndex = tileIndex * 6;

			result.vertices[vertexIndex].position		= { x * tileSize - centeringValue, z * tileSize - centeringValue, 0};
			result.vertices[vertexIndex + 1].position	= { (x + 1) * tileSize - centeringValue, z * tileSize - centeringValue, 0};
			result.vertices[vertexIndex + 2].position	= { x * tileSize - centeringValue, (z + 1) * tileSize - centeringValue, 0};
			result.vertices[vertexIndex + 3].position 	= { (x + 1) * tileSize - centeringValue, (z + 1) * tileSize - centeringValue, 0};

			bool32 isBlack = ((z % 2) + x) % 2; 
			Vector3 color = isBlack ? Vector3{0.25f, 0.25f, 0.25f} : Vector3{0.6f, 0.6f, 0.6f};

			result.vertices[vertexIndex].color = color;
			result.vertices[vertexIndex + 1].color = color;
			result.vertices[vertexIndex + 2].color = color;
			result.vertices[vertexIndex + 3].color = color;

			Vector3 normal = CoordinateSystem::Up;
			result.vertices[vertexIndex].normal = normal;
			result.vertices[vertexIndex + 1].normal = normal;
			result.vertices[vertexIndex + 2].normal = normal;
			result.vertices[vertexIndex + 3].normal = normal;

			result.vertices[vertexIndex].texCoord = {x * uvStep, z * uvStep};
			result.vertices[vertexIndex + 1].texCoord = {(x + 1) * uvStep, z * uvStep};
			result.vertices[vertexIndex + 2].texCoord = {x * uvStep, (z + 1) * uvStep};
			result.vertices[vertexIndex + 3].texCoord = {(x + 1) * uvStep, (z + 1) * uvStep};

			result.indices[triangleIndex] = vertexIndex;
			result.indices[triangleIndex + 1] = vertexIndex + 1;
			result.indices[triangleIndex + 2] = vertexIndex + 2;
			result.indices[triangleIndex + 3] = vertexIndex + 2;
			result.indices[triangleIndex + 4] = vertexIndex + 1;
			result.indices[triangleIndex + 5] = vertexIndex + 3;
		}
	}

	return result;

}