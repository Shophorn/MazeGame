/*=============================================================================
Leo Tamminen

Mesh Loader

Todo(Leo):
	- use reference variables for rapidjson things
=============================================================================*/

// Note(Leo): this also includes string and vector apparently
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
// #include <string>
// #include <vector>

struct GenericIterator
{
	u32 stride;
	u8 * pointer;
};

template <typename T>
T get_and_advance(GenericIterator * iterator)
{
	T result = *reinterpret_cast<T*>(iterator->pointer);
	iterator->pointer += iterator->stride;
	return result;
}

template <typename T>
T get_at(GenericIterator * iterator, u32 position)
{
	return *reinterpret_cast<T*>(iterator->pointer + (iterator->stride * position));
}

internal Skeleton
load_skeleton_glb(MemoryArena * memoryArena, char const * filePath, char const * modelName)
{
	using JsonValue = rapidjson::Value;

	auto rawBuffer 		= read_binary_file(memoryArena, filePath);
	auto jsonBuffer 	= get_gltf_json_chunk(rawBuffer, memoryArena);
	auto jsonString		= get_string(jsonBuffer);
	auto jsonDocument	= parse_json(jsonString.c_str());
	
	auto nodeArray = jsonDocument["nodes"].GetArray();
	
	JsonValue modelNode;
	for (auto & object : nodeArray)
	{
		auto name = object["name"].GetString();
		if (name == std::string(modelName))
		{
			assert(object.HasMember("skin"));

			modelNode = object;
			break;
		}
	}

	u32 skinIndex 	= modelNode["skin"].GetInt();
	auto skin 		= jsonDocument["skins"][skinIndex].GetObject();
	auto jointArray = skin["joints"].GetArray();
	u32 jointCount 	= jointArray.Size();

	Skeleton result = { push_array<Bone>(memoryArena, jointCount) };
	for (auto & bone : result.bones)
	{
		bone.boneSpaceTransform = {};
		bone.isRoot = true;
	}

	auto node_to_joint = [&](u32 nodeIndex) -> u32
	{
		/* Note(Leo): This is used so little it should not matter,
		but we could  do some clever look up table */
		for(u32 jointIndex = 0; jointIndex < jointCount; ++jointIndex)
		{
			if (jointArray[jointIndex].GetInt() == nodeIndex)
			{
				return jointIndex;
			}
		}
		assert(false && "Joint should have been found");
		return -1;
	};

	m44 gltf_to_mazegame = {1, 0, 0, 0,
							0, 0, 1, 0,
							0, -1, 0, 0,
							0, 0, 0, 1};

	for (int i = 0; i < jointCount; ++i)
	{
		u32 nodeIndex 		= jointArray[i].GetInt();
		auto const & node 	= nodeArray[nodeIndex].GetObject();
		if (node.HasMember("children"))
		{
			auto childArray = node["children"].GetArray();
			for(auto const & child : childArray)
			{
				u32 childIndex = node_to_joint(child.GetInt());
 				result.bones[childIndex].parent = i;
 				result.bones[childIndex].isRoot = false;
			}
		}
		
		if (i > 0)
		{
			if(node.HasMember("translation"))
			{
				auto translationArray = node["translation"].GetArray();
				result.bones[i].boneSpaceTransform.position.x = translationArray[0].GetFloat();
				result.bones[i].boneSpaceTransform.position.y = translationArray[1].GetFloat();
				result.bones[i].boneSpaceTransform.position.z = translationArray[2].GetFloat();

				result.bones[i].boneSpaceTransform.position = multiply_point(gltf_to_mazegame, result.bones[i].boneSpaceTransform.position);
			}

			if (node.HasMember("rotation"))
			{
				auto rotationArray = node["rotation"].GetArray();
				result.bones[i].boneSpaceTransform.rotation.vector.x = rotationArray[0].GetFloat();
				result.bones[i].boneSpaceTransform.rotation.vector.y = rotationArray[1].GetFloat();
				result.bones[i].boneSpaceTransform.rotation.vector.z = rotationArray[2].GetFloat();
				
				result.bones[i].boneSpaceTransform.rotation.vector = multiply_direction(gltf_to_mazegame, result.bones[i].boneSpaceTransform.rotation.vector);

				result.bones[i].boneSpaceTransform.rotation.w = rotationArray[3].GetFloat();


			}
		}
	}

	auto binaryBuffer = get_gltf_binary_chunk(memoryArena, rawBuffer, 0);

	auto get_iterator = [&](u32 accessorIndex) -> GenericIterator
	{
		auto accessor = jsonDocument["accessors"][accessorIndex].GetObject();
		auto componentType = (glTF::ComponentType)accessor["componentType"].GetInt();
		u32 size = glTF::get_component_size(componentType);
		
		u32 componentCount = glTF::get_component_count(accessor["type"].GetString());

		u32 viewIndex = accessor["bufferView"].GetInt();
		u32 offset = jsonDocument["bufferViews"][viewIndex]["byteOffset"].GetInt();

		return {.stride = size * componentCount,
				.pointer = binaryBuffer.begin() + offset};

	};

	auto bindMatrixIterator = get_iterator(skin["inverseBindMatrices"].GetInt());


	m44 modelSpaceTransforms [20] = {};
	result.bones[0].inverseBindMatrix = matrix::make_identity<m44>();
	modelSpaceTransforms[0] = matrix::make_identity<m44>();
	for (int i = 1; i < result.bones.count(); ++i)
	{
		m44 boneSpaceBindTransform = get_matrix(result.bones[i].boneSpaceTransform);
		u32 parentIndex = result.bones[i].parent;
		m44 modelSpaceBindTransform = matrix::multiply(modelSpaceTransforms[parentIndex], boneSpaceBindTransform);
		result.bones[i].inverseBindMatrix = invert_transform_matrix(modelSpaceBindTransform);
		modelSpaceTransforms[i] = modelSpaceBindTransform;
	}

	// for(auto & bone : result.bones)
	// {
	// 	bone.inverseBindMatrix = get_and_advance<m44>(&bindMatrixIterator);
	// 	bone.inverseBindMatrix = matrix::multiply(gltf_to_mazegame, bone.inverseBindMatrix);
	// }

	std::cout << "[load_skeleton_glb()]: root inverseBindMatrix = " << result.bones[0].inverseBindMatrix << "\n";

	// result.bones[0].isRoot = true;			// Root
	// result.bones[1].parent = 0;			// Hip
	// result.bones[2].parent = 1;			// Back
	// result.bones[11].parent = 1;			// Thigh L
	// result.bones[14].parent = 1;			// Thigh R
	// result.bones[3].parent = 2;			// Neck
	// result.bones[8].parent = 2;			// Arm R
	// result.bones[5].parent = 2;			// Arm L
	// result.bones[4].parent = 3;			// Head
	// result.bones[6].parent = 5;			// ForeArm L
	// result.bones[7].parent = 6;			// Hand L
	// result.bones[9].parent = 8;			// ForeArm R
	// result.bones[10].parent = 9;			// Hand R
	// result.bones[12].parent = 11;		// Shin L
	// result.bones[13].parent = 12;		// Foot L
	// result.bones[15].parent = 14;		// Shin R
	// result.bones[16].parent = 15;		// Foot R

	return result;
}

internal MeshAsset
load_model_glb(MemoryArena * memoryArena, char const * filePath, char const * modelName)
{
	using JsonValue = rapidjson::Value;
	/*
	Todo(Leo): 
		- multiple objects in file
		- other hierarchies
		- world transforms
		- sparse accessor
	*/

	auto rawBuffer 		= read_binary_file(memoryArena, filePath);
	auto jsonBuffer 	= get_gltf_json_chunk(rawBuffer, memoryArena);
	auto jsonString		= get_string(jsonBuffer);
	auto jsonDocument	= parse_json(jsonString.c_str());
	
	auto nodeArray = jsonDocument["nodes"].GetArray();
	
	s32 positionAccessorIndex 	= -1; 
	s32 normalAccessorIndex 	= -1;
	s32 texcoordAccessorIndex 	= -1;

	s32 bonesAccessorIndex 			= -1;
	s32 boneWeightsAccessorIndex 	= -1;

	JsonValue modelNode;

	s32 indexAccessorIndex 		= -1;

	for (auto & object : nodeArray)
	{
		auto name = object["name"].GetString();
		if (name == std::string(modelName))
		{
			assert(object.HasMember("mesh"));

			modelNode = object;
			break;
		}
	}

	u32 meshIndex 			= modelNode["mesh"].GetInt();
	auto meshObject			= jsonDocument["meshes"][0].GetObject();
	auto primitivesArray 	= meshObject["primitives"].GetArray();

	auto attribObject 		= primitivesArray[0].GetObject()["attributes"].GetObject();
	positionAccessorIndex 	= attribObject["POSITION"].GetInt();
	normalAccessorIndex 	= attribObject["NORMAL"].GetInt();
	texcoordAccessorIndex 	= attribObject["TEXCOORD_0"].GetInt();

	if (attribObject.HasMember("JOINTS_0"))
		bonesAccessorIndex 		= attribObject["JOINTS_0"].GetInt();

	if (attribObject.HasMember("WEIGHTS_0"))
		boneWeightsAccessorIndex = attribObject["WEIGHTS_0"].GetInt();

	indexAccessorIndex 		= primitivesArray[0].GetObject()["indices"].GetInt();


	auto accessorArray 		= jsonDocument["accessors"].GetArray();
	auto bufferViewArray 	= jsonDocument["bufferViews"].GetArray();
	auto buffersArray 		= jsonDocument["buffers"].GetArray();
	
	// Todo(Leo): what if there are multiple buffers in file? Where is the index? In "buffers"?
	auto binaryBuffer 			= get_gltf_binary_chunk(memoryArena, rawBuffer, 0);

	auto get_buffer_view_index = [&accessorArray](s32 index) -> s32
	{
		s32 result = accessorArray[index]["bufferView"].GetInt();
		return result;
	};

	auto get_item_count = [&accessorArray](s32 index) -> u64
	{
		u64 result = accessorArray[index]["count"].GetInt();
		return result;
	};

	s32 indexBufferViewIndex 	= get_buffer_view_index(indexAccessorIndex);

	u64 vertexCount 			= get_item_count(positionAccessorIndex);
	bool32 isGood 				= 	(vertexCount == get_item_count(normalAccessorIndex))
									&& (vertexCount == get_item_count(texcoordAccessorIndex));
	DEBUG_ASSERT(isGood, "Did not read .glb properly: invalid vertex count among properties or invalid accessor indices.");

	// -----------------------------------------------------------------------------
	auto get_buffer_byte_offset = [&](u32 index)
	{
		auto bufferView = bufferViewArray[index].GetObject();
		assert(bufferView.HasMember("byteOffset"));
		return bufferView["byteOffset"].GetInt();
	};

	auto vertices 			= push_array<Vertex>(memoryArena, vertexCount);

	auto get_iterator = [&](u32 accessorIndex) -> GenericIterator
	{
		auto accessor = accessorArray[accessorIndex].GetObject();
		auto componentType = (glTF::ComponentType)accessor["componentType"].GetInt();
		
		u32 size = glTF::get_component_size(componentType);
		u32 componentCount = glTF::get_component_count(accessor["type"].GetString());

		u32 viewIndex = get_buffer_view_index(accessorIndex);
		u32 offset = get_buffer_byte_offset(viewIndex);

		return {.stride = size * componentCount,
				.pointer = binaryBuffer.begin() + offset};

	};


	auto posIt 			= get_iterator(positionAccessorIndex);
	auto normalIt 		= get_iterator(normalAccessorIndex);
	auto texcoordIt 	= get_iterator(texcoordAccessorIndex);

	for (auto & vertex : vertices)
	{
		vertex.position = get_and_advance<v3>(&posIt);
		vertex.normal 	= get_and_advance<v3>(&normalIt);
		vertex.texCoord = get_and_advance<v2>(&texcoordIt);
	}

	bool32 hasSkin = bonesAccessorIndex >= 0 && boneWeightsAccessorIndex >= 0;
	if (hasSkin)
	{
		auto bonesIterator = get_iterator(bonesAccessorIndex);
		auto weightIterator = get_iterator(boneWeightsAccessorIndex);

		for (auto & vertex : vertices)
		{
			vertex.boneIndices = get_and_advance<upoint4>(&bonesIterator);
			vertex.boneWeights = get_and_advance<v4>(&weightIterator);
			vector::normalize(vertex.boneWeights);
		}
	}

	///// INDICES //////
	auto indexAccessor 	= accessorArray[indexAccessorIndex].GetObject();
	u64 indexCount 		= indexAccessor["count"].GetInt();
	auto indices 		= push_array<u16>(memoryArena, indexCount);

	auto indexIterator = get_iterator(indexAccessorIndex);
	for (u16 & index : indices)
	{
		index = get_and_advance<u16>(&indexIterator);
	}

	MeshAsset result = make_mesh_asset(vertices, indices);
	return result;
}

internal MeshAsset
load_model_obj(MemoryArena * memoryArena, char const * modelPath)
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
	s32 indexCount = shapes[0].mesh.indices.size();

	result.vertices = push_array<Vertex>(memoryArena, indexCount);
	result.indices = push_array<u16>(memoryArena, indexCount);

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
	transform(MeshAsset * mesh, const m44 & transform)
	{
		int vertexCount = mesh->vertices.count();
		for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			mesh->vertices[vertexIndex].position = multiply_point(transform, mesh->vertices[vertexIndex].position); 
		}
	}

	internal void
	transform_tex_coords(MeshAsset * mesh, const v2 translation, const v2 scale)
	{
		int vertexCount = mesh->vertices.count();
		for(int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{

			mesh->vertices[vertexIndex].texCoord = vector::scale(mesh->vertices[vertexIndex].texCoord, scale);
			mesh->vertices[vertexIndex].texCoord += translation;
		}
	}
}

namespace mesh_primitives
{
	constexpr real32 radius = 0.5f;

	MeshAsset create_quad(MemoryArena * memoryArena, bool32 flipIndices)
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

		if (flipIndices)
		{
			// Note(Leo): These seem to backwardsm but it because currently our gui is viewed from behind.
			result.indices = push_array<u16>(memoryArena, {0, 2, 1, 1, 2, 3});
		}
		else
		{
			result.indices = push_array<u16>(memoryArena, {0, 1, 2, 2, 1, 3});
		}

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