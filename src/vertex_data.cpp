#include "Array.hpp"
	
// Note (Leo): Only for tinyobj errors
#include <string>

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription
	GetBindingDescription ()
	{
		VkVertexInputBindingDescription value = {};

		value.binding = 0;
		value.stride = sizeof(Vertex);
		value.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Note(Leo): Other option here has to do with instancing

		return value;
	}

	static Array<VkVertexInputAttributeDescription, 3>
	GetAttributeDescriptions()
	{
		Array<VkVertexInputAttributeDescription, 3> value = {};

		value[0].binding = 0;
		value[0].location = 0;
		value[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		value[0].offset = offsetof(Vertex, position);

		value[1].binding = 0;
		value[1].location = 1;
		value[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		value[1].offset = offsetof(Vertex, color);	

		value[2].binding = 0;
		value[2].location = 2;
		value[2].format = VK_FORMAT_R32G32_SFLOAT;
		value[2].offset = offsetof(Vertex, texCoord);

		return value;
	}	
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<uint16> indices;
	VkIndexType indexType;
};

Mesh generatedMap;
Mesh characterModel;

/* Todo(Leo): Add target pointer to memory as optional argument, so we can load 
this directly where it is used */
Mesh
GenerateMap()
{
	Mesh result = {};
	result.indexType = VK_INDEX_TYPE_UINT16;

	int tileCountPerDirection = 15;
	float tileSize = 0.1f;
	float centeringValue = tileSize * tileCountPerDirection / 2.0f;
	
	int vertexCountPerDirection = tileCountPerDirection * 2;
	int vertexCount = vertexCountPerDirection * vertexCountPerDirection;
	result.vertices.resize(vertexCount);

	int indexCount = tileCountPerDirection * tileCountPerDirection * 6;
	result.indices.resize(indexCount);	

	for (int z = 0; z < tileCountPerDirection; ++z)
	{
		for (int x = 0; x < tileCountPerDirection; ++x)
		{
			int tileIndex = x + z * tileCountPerDirection;
			int vertexIndex = tileIndex * 4;
			int triangleIndex = tileIndex * 6;

			result.vertices[vertexIndex].position		= { x * tileSize - centeringValue, 	0,  z * tileSize - centeringValue};
			result.vertices[vertexIndex + 1].position	= { (x + 1) * tileSize - centeringValue, 0,  z * tileSize - centeringValue};
			result.vertices[vertexIndex + 2].position	= { x * tileSize - centeringValue, 	0, (z + 1) * tileSize - centeringValue};
			result.vertices[vertexIndex + 3].position 	= { (x + 1) * tileSize - centeringValue, 0, (z + 1) * tileSize - centeringValue};

			bool32 isBlack = ((z % 2) + x) % 2; 
			glm::vec3 color = isBlack ? glm::vec3{0.25f, 0.25f, 0.25f} : glm::vec3{0.6f, 0.6f, 0.6f};

			result.vertices[vertexIndex].color = color;
			result.vertices[vertexIndex + 1].color = color;
			result.vertices[vertexIndex + 2].color = color;
			result.vertices[vertexIndex + 3].color = color;

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

Mesh
LoadModel(const char * model_path)
{
	Mesh result = {};
	result.indexType = VK_INDEX_TYPE_UINT16;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, model_path) == false)
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

			result.vertices.push_back(vertex);
			result.indices.push_back(result.indices.size());
		}
	}

	return result;
}

// Note(Leo): these need to align properly
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 projection;
};