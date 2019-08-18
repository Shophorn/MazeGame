
// Note (Leo): Only for tinyobj errors
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

/* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
this directly where it is used */
inline Mesh
GenerateMap()
{
	Mesh result = {};
	result.indexType = IndexType::UInt16;

	int tileCountPerDirection = 128;
	float tileSize = 1.0f;
	float centeringValue = tileSize * tileCountPerDirection / 2.0f;
	
	int vertexCountPerDirection = tileCountPerDirection * 2;
	int vertexCount = vertexCountPerDirection * vertexCountPerDirection;
	result.vertices.resize(vertexCount);

	int indexCount = tileCountPerDirection * tileCountPerDirection * 6;
	result.indices.resize(indexCount);	

	real32 uvStep = 1.0f / (real32)(tileCountPerDirection + 1);

	for (int z = 0; z < tileCountPerDirection; ++z)
	{
		for (int x = 0; x < tileCountPerDirection; ++x)
		{
			int tileIndex = x + z * tileCountPerDirection;
			int vertexIndex = tileIndex * 4;
			int triangleIndex = tileIndex * 6;

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

			Vector3 normal = {0, 1, 0};
			result.vertices[vertexIndex].normal = normal;
			result.vertices[vertexIndex + 1].normal = normal;
			result.vertices[vertexIndex + 2].normal = normal;
			result.vertices[vertexIndex + 3].normal = normal;

			result.vertices[vertexIndex].texCoord = {x * uvStep, z * uvStep};
			result.vertices[vertexIndex + 1].texCoord = {(x + 1) * uvStep, z * uvStep};
			result.vertices[vertexIndex + 2].texCoord = {x * uvStep, (z + 1) * uvStep};
			result.vertices[vertexIndex + 3].texCoord = {(x + 1) * uvStep, (z + 1) * uvStep};

			result.indices[triangleIndex] = vertexIndex;
			result.indices[triangleIndex + 1] = vertexIndex + 2;
			result.indices[triangleIndex + 2] = vertexIndex + 1;
			result.indices[triangleIndex + 3] = vertexIndex + 1;
			result.indices[triangleIndex + 4] = vertexIndex + 2;
			result.indices[triangleIndex + 5] = vertexIndex + 3;
		}
	}

	return result;

}

inline Mesh
LoadModel(const char * modelPath)
{
	/*
	TODO(Leo): There is now no checking if attributes exist.
	This means all below attributes must exist in file.
	Of course in final game we know beforehand what there is.
	*/

	Mesh result = {};
	result.indexType = IndexType::UInt16;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelPath) == false)
	{
		throw std::runtime_error(warning + error);
	}

	for (const auto & shape : shapes)
	{
		for (const auto & index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.position = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = {0.8f, 1.0f, 1.0f};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			result.vertices.push_back(vertex);
			result.indices.push_back(result.indices.size());
		}
	}

	return result;
}


namespace MeshPrimitives
{
	constexpr real32 radius = 0.5f;

	Mesh cube = {
		/// VERTICES
		{
			/// LEFT face
			{-radius, -radius,  radius, -1, 0, 0, 1, 1, 1, 0, 0},
			{-radius, -radius, -radius, -1, 0, 0, 1, 1, 1, 1, 0},
			{-radius,  radius,  radius, -1, 0, 0, 1, 1, 1, 0, 1},
			{-radius,  radius, -radius, -1, 0, 0, 1, 1, 1, 1, 1},

			/// RIGHT face
			{radius, -radius, -radius, 1, 0, 0, 1, 1, 1, 0, 0},
			{radius, -radius,  radius, 1, 0, 0, 1, 1, 1, 1, 0},
			{radius,  radius, -radius, 1, 0, 0, 1, 1, 1, 0, 1},
			{radius,  radius,  radius, 1, 0, 0, 1, 1, 1, 1, 1},
	
			/// BACK face
			{-radius, -radius,  radius, 0, -1, 0, 1, 1, 1, 0, 0},
			{ radius, -radius,  radius, 0, -1, 0, 1, 1, 1, 1, 0},
			{-radius, -radius, -radius, 0, -1, 0, 1, 1, 1, 0, 1},
			{ radius, -radius, -radius, 0, -1, 0, 1, 1, 1, 1, 1},

			/// FORWARD face
			{-radius, radius, -radius, 0, 1, 0, 1, 1, 1, 0, 0},
			{ radius, radius, -radius, 0, 1, 0, 1, 1, 1, 1, 0},
			{-radius, radius,  radius, 0, 1, 0, 1, 1, 1, 0, 1},
			{ radius, radius,  radius, 0, 1, 0, 1, 1, 1, 1, 1},

			/// BOTTOM face
			{-radius, -radius, -radius, 0, 0, -1, 1, 1, 1, 0, 0},
			{ radius, -radius, -radius, 0, 0, -1, 1, 1, 1, 1, 0},
			{-radius,  radius, -radius, 0, 0, -1, 1, 1, 1, 0, 1},
			{ radius,  radius, -radius, 0, 0, -1, 1, 1, 1, 1, 1},

			/// UP face
			{ radius, -radius, radius, 0, 0, 1, 1, 1, 1, 0, 0},
			{-radius, -radius, radius, 0, 0, 1, 1, 1, 1, 1, 0},
			{ radius,  radius, radius, 0, 0, 1, 1, 1, 1, 0, 1},
			{-radius,  radius, radius, 0, 0, 1, 1, 1, 1, 1, 1}
		},

		/// INDICES
		{
			/// LEFT face
			0, 2, 1, 1, 2, 3,

			/// RIGHT face
			4, 6, 5, 5, 6, 7,
	
			/// BOTTOM face
			8, 10, 9, 9, 10, 11,

			/// TOP face
			12, 14, 13, 13, 14, 15,

			/// BACK face
			16, 18, 17, 17, 18, 19,

			/// FRONT face
			20, 22, 21, 21, 22, 23
		},
		IndexType::UInt16
	};
}