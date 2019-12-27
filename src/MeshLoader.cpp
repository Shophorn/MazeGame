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
load_model_glb(MemoryArena * memoryArena, const char * filePath, const char * modelName)
{
	/*
	Todo(Leo): 
		- stride
		- sparse accessor
	*/

	auto gltf 			= read_binary_file(memoryArena, filePath);
	auto jsonBuffer 	= get_gltf_json_chunk(gltf, memoryArena);
	auto jsonString		= get_string(jsonBuffer);
	auto jsonDocument	= parse_json(jsonString.c_str());
	
	auto meshArray = jsonDocument["meshes"].GetArray();
	
	int32 positionAccessorIndex 	= -1; 
	int32 normalAccessorIndex 		= -1;
	int32 texcoordAccessorIndex 	= -1;
	int32 indexAccessorIndex 		= -1;

	for (auto & object : meshArray)
	{
		auto name = object["name"].GetString();
		if (name == std::string(modelName))
		{

			auto primitivesArray 	= object["primitives"].GetArray();

			auto attribObject 		= primitivesArray[0].GetObject()["attributes"].GetObject();
			positionAccessorIndex 	= attribObject["POSITION"].GetInt();
			normalAccessorIndex 	= attribObject["NORMAL"].GetInt();
			texcoordAccessorIndex 	= attribObject["TEXCOORD_0"].GetInt();

			indexAccessorIndex 		= primitivesArray[0].GetObject()["indices"].GetInt();

			break;
		}
	}

	std::cout 	<< "[GLTF]: Accessor indices\n"
				<< "\tPOSITION: " << positionAccessorIndex << "\n"	
				<< "\tNORMAL: " << normalAccessorIndex << "\n"	
				<< "\tTEXCOORD_0: " << texcoordAccessorIndex << "\n"	
				<< "\tindices: " << indexAccessorIndex << "\n";

	auto accessorArray 		= jsonDocument["accessors"].GetArray();
	auto bufferViewArray 	= jsonDocument["bufferViews"].GetArray();
	auto buffersArray 		= jsonDocument["buffers"].GetArray();
	
	// Todo(Leo): what if there are multiple buffers in file? Where is the index? In "buffers"?
	auto buffer = get_gltf_binary_chunk(memoryArena, gltf, 0);

	auto get_buffer_view_index = [&accessorArray](int32 index) -> int32
	{
		int32 result = accessorArray[index].GetObject()["bufferView"].GetInt();
		return result;
	};

	auto get_item_count = [&accessorArray](int32 index) -> uint64
	{
		uint64 result = accessorArray[index].GetObject()["count"].GetInt();
		return result;
	};

	int32 positionBufferViewIndex 	= get_buffer_view_index(positionAccessorIndex);
	int32 normalBufferViewIndex 	= get_buffer_view_index(normalAccessorIndex);
	int32 texcoordBufferViewIndex 	= get_buffer_view_index(texcoordAccessorIndex);
	int32 indexBufferViewIndex 		= get_buffer_view_index(indexAccessorIndex);

	uint64 vertexCount 	= get_item_count(positionAccessorIndex);
	bool32 isGood 		= 	(vertexCount == get_item_count(normalAccessorIndex))
							&& (vertexCount == get_item_count(texcoordAccessorIndex));
	MAZEGAME_ASSERT(isGood, "Did not read .glb properly: invalid vertex count among properties or invalid accessor indices.");

	////// VERTICES ///////
	std::cout << "[GLTF vertices]: 1\n";

	auto positionAccessor 	= accessorArray[positionAccessorIndex].GetObject();
	auto normalAccessor 	= accessorArray[normalAccessorIndex].GetObject();
	auto texcoordAccessor 	= accessorArray[texcoordAccessorIndex].GetObject();

	auto positionBufferView = bufferViewArray[positionBufferViewIndex].GetObject();
	auto normalBufferView 	= bufferViewArray[normalBufferViewIndex].GetObject();
	auto texcoordBufferView = bufferViewArray[texcoordBufferViewIndex].GetObject();

	std::cout << "[GLTF vertices]: 2\n";
	uint64 positionOffset 		= positionBufferView["byteOffset"].GetInt();
	uint64 positionLength		= positionBufferView["byteLength"].GetInt(); 
	auto positionComponentType 	= (glTF::ComponentType)positionAccessor["componentType"].GetInt();
	uint64 positionStride		= glTF::get_default_stride(positionComponentType);

	std::cout << "[GLTF vertices]: 3\n";
	uint64 normalOffset 		= normalBufferView["byteOffset"].GetInt();
	uint64 normalLength			= normalBufferView["byteLength"].GetInt(); 
	auto normalComponentType 	= (glTF::ComponentType)normalAccessor["componentType"].GetInt();
	uint64 normalStride			= glTF::get_default_stride(normalComponentType);

	std::cout << "[GLTF vertices]: 4\n";
	uint64 texcoordOffset 		= texcoordBufferView["byteOffset"].GetInt();
	uint64 texcoordLength		= texcoordBufferView["byteLength"].GetInt(); 
	auto texcoordComponentType 	= (glTF::ComponentType)texcoordAccessor["componentType"].GetInt();
	uint64 texcoordStride		= glTF::get_default_stride(texcoordComponentType);

	std::cout << "[GLTF vertices]: 5\n";
	auto vertices 				= reserve_array<Vertex>(memoryArena, vertexCount);

	uint64 positionStart 		= positionOffset;
	uint64 normalStart 			= normalOffset;
	uint64 texcoordStart 		= texcoordOffset;

	Vector3 * positionBuffer 	= reinterpret_cast<Vector3*>(buffer.begin() + positionOffset);
	Vector3 * normalBuffer 		= reinterpret_cast<Vector3*>(buffer.begin() + normalOffset);
	Vector2 * texcoordBuffer 	= reinterpret_cast<Vector2*>(buffer.begin() + texcoordOffset);

	for (int i = 0; i < vertexCount; ++i)
	{
		Vector3 position 	= positionBuffer[i];
		Vector3 normal 		= normalBuffer[i];
		Vector2 texCoord    = texcoordBuffer[i];

		push_one(&vertices, Vertex{
			.position 	= position,
			.normal 	= normal,
			.texCoord 	= texCoord});
	}
	std::cout << "[GLTF vertices]: 5\n";

	///// INDICES //////
	auto indexAccessor 		= accessorArray[indexAccessorIndex].GetObject();
	auto indexBufferView 	= bufferViewArray[indexBufferViewIndex].GetObject();


	uint64 indexCount 		= indexAccessor["count"].GetInt();
	auto indices 			= reserve_array<uint16>(memoryArena, indexCount);

	uint64 indexByteOffset 	= indexBufferView["byteOffset"].GetInt();
	uint64 indexByteLength 	= indexBufferView["byteLength"].GetInt();

	byte * indexStart 		= buffer.begin() + indexByteOffset;
	byte * indexEnd 		= indexStart + indexByteLength;
	auto componentType		= (glTF::ComponentType)indexAccessor["componentType"].GetInt();
	uint64 stride			= glTF::get_default_stride(componentType);

	std::cout << "[GLTF]: Index stride = " << stride << "\n";

	for(byte * it = indexStart; it < indexEnd; it += stride)
	{
		uint16 index = *reinterpret_cast<uint16*>(it);
		push_one(&indices, index);
	}

	std::cout << "[GLTF]: indices read properly\n";


	MeshAsset result = make_mesh_asset(vertices, indices);
	return result;
}

internal MeshAsset
load_model_obj(MemoryArena * memoryArena, const char * modelPath)
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

	result.vertices = push_array<Vertex>(memoryArena, indexCount);
	result.indices = push_array<uint16>(memoryArena, indexCount);

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

namespace mesh_ops
{
	internal void
	transform(MeshAsset * mesh, const Matrix44 & transform)
	{
		int vertexCount = mesh->vertices.count();
		for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			mesh->vertices[vertexIndex].position = transform * mesh->vertices[vertexIndex].position; 
		}
	}

	internal void
	transform_tex_coords(MeshAsset * mesh, const Vector2 translation, const Vector2 scale)
	{
		int vertexCount = mesh->vertices.count();
		for(int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			mesh->vertices[vertexIndex].texCoord *= scale;
			mesh->vertices[vertexIndex].texCoord += translation;
		}
	}
}

namespace mesh_primitives
{
	constexpr real32 radius = 0.5f;

	MeshAsset create_quad(MemoryArena * memoryArena)
	{
		MeshAsset result = {};
		result.indexType = IndexType::UInt16;

		int vertexCount = 4;
		result.vertices = push_array<Vertex>(memoryArena, vertexCount);

		// 					  position 		normal   	color    	uv
		result.vertices[0] = {0, 0, 0, 		0, 0, 1, 	1, 1, 1, 	0, 0};
		result.vertices[1] = {1, 0, 0, 		0, 0, 1, 	1, 1, 1, 	1, 0};
		result.vertices[2] = {0, 1, 0, 		0, 0, 1, 	1, 1, 1, 	0, 1};
		result.vertices[3] = {1, 1, 0, 		0, 0, 1, 	1, 1, 1, 	1, 1};

		int indexCount = 6;
		result.indices = push_array<uint16>(memoryArena, indexCount);

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