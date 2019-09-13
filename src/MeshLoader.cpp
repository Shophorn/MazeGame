/*=============================================================================
Leo Tamminen

Mesh Loader

Todo[Assets](Leo): look into gltf format or make own. 
=============================================================================*/

// Note(Leo): this also includes string and vector apparently
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
// #include <string>
// #include <vector>

internal MeshAsset
LoadModel(MemoryArena * memoryArena, const char * modelPath)
{
	/*
	TODO(Leo): There is now no checking if attributes exist.
	This means all below attributes must exist in file.
	Of course in final game we know beforehand what there is.
	*/

	MeshAsset result = {};
	result.indexType = IndexType::UInt16;

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warning, error;

	if (tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, modelPath) == false)
	{
		throw std::runtime_error(warning + error);
	}

	//Note(Leo): there might be more than one shape, but we don't use that here
	int32 indexCount = shapes[0].mesh.indices.size();

	result.vertices = PushArray<Vertex>(memoryArena, indexCount);
	result.indices = PushArray<uint16>(memoryArena, indexCount);

	for (int i = 0; i < indexCount; ++i)
	{
		Vertex vertex = {};

		vertex.position = {
			attrib.vertices[3 * shapes[0].mesh.indices[i].vertex_index + 0],
			attrib.vertices[3 * shapes[0].mesh.indices[i].vertex_index + 1],
			attrib.vertices[3 * shapes[0].mesh.indices[i].vertex_index + 2]
		};

		vertex.texCoord = {
			attrib.texcoords[2 * shapes[0].mesh.indices[i].texcoord_index + 0],
			1.0f - attrib.texcoords[2 * shapes[0].mesh.indices[i].texcoord_index + 1]
		};

		vertex.color = {0.8f, 1.0f, 1.0f};

		vertex.normal = {
			attrib.normals[3 * shapes[0].mesh.indices[i].normal_index + 0],
			attrib.normals[3 * shapes[0].mesh.indices[i].normal_index + 1],
			attrib.normals[3 * shapes[0].mesh.indices[i].normal_index + 2]
		};

		result.vertices[i] = vertex;
		result.indices[i] = i;

	}

	return result;
}


namespace mesh_primitives
{
	constexpr real32 radius = 0.5f;

	MeshAsset CreateQuad(MemoryArena * memoryArena)
	{
		MeshAsset result = {};
		result.indexType = IndexType::UInt16;

		int vertexCount = 4;
		result.vertices = PushArray<Vertex>(memoryArena, vertexCount);

		// 					   position			   normal 	color 	 uv
		result.vertices[0] = {-radius, -radius, 0, 0, 0, 1, 1, 1, 1, 0, 0};
		result.vertices[1] = { radius, -radius, 0, 0, 0, 1, 1, 1, 1, 1, 0};
		result.vertices[2] = {-radius,  radius, 0, 0, 0, 1, 1, 1, 1, 0, 1};
		result.vertices[3] = { radius,  radius, 0, 0, 0, 1, 1, 1, 1, 1, 1};


		int indexCount = 6;
		result.indices = PushArray<uint16>(memoryArena, indexCount);

		result.indices [0] = 0;
		result.indices [1] = 1;
		result.indices [2] = 2;
		result.indices [3] = 2;
		result.indices [4] = 1;
		result.indices [5] = 3;


		return result;
	}

	/*
	MeshAsset cube = {
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
	*/
}