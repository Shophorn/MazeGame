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

struct GltfIterator
{
	int const stride;
	int const count;
	void const * const start;

	int position = 0;
};

GltfIterator
make_gltf_iterator (BinaryBuffer const & buffer, rapidjson::Document const & document, u32 accessorIndex)
{
	auto accessor = document["accessors"][accessorIndex].GetObject();
	auto componentType = (glTF::ComponentType)accessor["componentType"].GetInt();
	u32 size = glTF::get_component_size(componentType);
	
	u32 componentCount = glTF::get_component_count(accessor["type"].GetString());

	u32 viewIndex = accessor["bufferView"].GetInt();
	u32 offset = document["bufferViews"][viewIndex]["byteOffset"].GetInt();

	u32 count = accessor["count"].GetInt();

	return {.stride 	= (int)(size * componentCount),
			.count 		= (int)count,
			.start 		= buffer.data() + offset};

};


template <typename T>
T get_and_advance(GltfIterator & iterator)
{
	T result = *reinterpret_cast<T const*>((u8*)iterator.start + iterator.position);
	// iterator.start = (u8*)iterator.start + iterator.stride;
	iterator.position += iterator.stride;
	return result;
}

template <typename T>
T get_at(GltfIterator const & iterator, u32 position)
{
	return *reinterpret_cast<T const*>((u8*)iterator.start + (iterator.stride * position));
}

int find_bone_by_node(rapidjson::Document const & jsonDocument, int nodeIndex)
{
	assert(jsonDocument.HasMember("skins"));
	assert(jsonDocument["skins"].Size() == 1);
	assert(jsonDocument["skins"][0].HasMember("joints"));

	auto bonesArray = jsonDocument["skins"][0]["joints"].GetArray();


	int boneCount = bonesArray.Size();

	for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		int nodeForBone = bonesArray[boneIndex].GetInt();

		if (nodeForBone == nodeIndex)
			return boneIndex;
	}

	return -1;
}

using JsonValue = rapidjson::Value;
using JsonArray = rapidjson::GenericArray<true, JsonValue>;

auto find_by_name (JsonArray & nodeArray, char const * name)
	-> JsonValue const *
{
	for (auto const & node : nodeArray)
	{
		if (cstring_equals(node["name"].GetString(), name))
		{
			return &node;
		}
	}
	return nullptr;
};

internal Animation
load_animation_glb(MemoryArena & memoryArena, GltfFile const & gltfFile, char const * animationName)
{
	auto const & jsonDocument = gltfFile.json;

	DEBUG_ASSERT(jsonDocument.HasMember("animations"), "No Animations found");

	auto animArray = jsonDocument["animations"].GetArray();

	JsonValue const * ptrNamedAnimation = find_by_name(animArray, animationName);

	if(ptrNamedAnimation == nullptr)
	{
		std::cout << "Animation not found. Requested: " << animationName << ", available:\n";
		for (auto const & anim : animArray)
		{
			std::cout << "\t" << anim["name"].GetString() << "\n";
		}
		DEBUG_ASSERT(ptrNamedAnimation != nullptr, "animation not found");
	}

	auto const & namedAnimation = *ptrNamedAnimation;

	auto channels 		= namedAnimation["channels"].GetArray();
	auto samplers 		= namedAnimation["samplers"].GetArray();

	auto accessors 		= jsonDocument["accessors"].GetArray();
	auto bufferViews 	= jsonDocument["bufferViews"].GetArray();
	auto buffers 		= jsonDocument["buffers"].GetArray();

	Animation result = {};
	result.boneAnimations = allocate_array<BoneAnimation>(memoryArena, channels.Size());

	float animationMinTime = math::highest_value<float>;
	float animationMaxTime = math::lowest_value<float>;

	for (auto const & channel : channels)
	{

		int samplerIndex = channel["sampler"].GetInt();
		int inputAccessorIndex = samplers[samplerIndex]["input"].GetInt();
		int outputAccessorIndex = samplers[samplerIndex]["output"].GetInt();

		animationMinTime = math::min(animationMinTime, accessors[inputAccessorIndex]["min"][0].GetFloat());
		animationMaxTime = math::max(animationMaxTime, accessors[inputAccessorIndex]["max"][0].GetFloat());

		int inputBufferViewIndex = accessors[inputAccessorIndex]["bufferView"].GetInt();
		int outputBufferViewIndex = accessors[outputAccessorIndex]["bufferView"].GetInt();

		int keyframeCount = accessors[inputAccessorIndex]["count"].GetInt();
		
		auto inputIt 	= make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, inputAccessorIndex);
		auto outputIt 	= make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, outputAccessorIndex);

		auto targetNode 	= channel["target"]["node"].GetInt();
		char const * path 	= channel["target"]["path"].GetString();

		BoneAnimation boneAnimation = {};
		boneAnimation.boneIndex = find_bone_by_node(jsonDocument, targetNode);
		

		char const * interpolationMode = samplers[samplerIndex]["interpolation"].GetString();
		std::cout << "interpolation mode = " << interpolationMode << "\n";

		if(cstring_equals(interpolationMode, "STEP"))
		{
			boneAnimation.interpolationMode = InterpolationMode::Step;
		}
		else if (cstring_equals(interpolationMode, "LINEAR"))
		{
			boneAnimation.interpolationMode = InterpolationMode::Linear;
		}
		else if (cstring_equals(interpolationMode,"CUBICSPLINE"))
		{
			boneAnimation.interpolationMode = InterpolationMode::CubicSpline;
		}
		else
		{
			assert(false);
		}

		if (cstring_equals(path, "rotation"))
		{
			std::cout << "rotation channel for bone " << boneAnimation.boneIndex;
		
			boneAnimation.rotations = allocate_array<RotationKeyframe>(memoryArena, keyframeCount);
			for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
			{
				int actualIndex = boneAnimation.interpolationMode == InterpolationMode::CubicSpline ? (keyframeIndex * 3 + 1) : keyframeIndex;

				float time 			= get_at<float>(inputIt, keyframeIndex);
				quaternion rotation = get_at<quaternion>(outputIt, actualIndex);

				// TODO(Leo): Why does this work?
				rotation 			= rotation.inverse();

				boneAnimation.rotations.push({time, rotation});
			}

			std::cout << ", succeeded\n";
		}

		else if (cstring_equals(path, "translation"))
		{

			std::cout << "position channel for bone " << boneAnimation.boneIndex;

			boneAnimation.positions = allocate_array<PositionKeyframe>(memoryArena, keyframeCount);
			for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
			{
				int actualIndex = boneAnimation.interpolationMode == InterpolationMode::CubicSpline ? (keyframeIndex * 3 + 1) : keyframeIndex;
	
				float time 	= get_at<float>(inputIt, keyframeIndex);
				v3 position = get_at<v3>(outputIt, actualIndex);


				boneAnimation.positions.push({time, position});
			}
			std::cout << ", succeeded\n";
		}
		result.boneAnimations.push(std::move(boneAnimation));
	}
	result.duration = animationMaxTime - animationMinTime;

	std::cout << "Animation loaded, duration: " << result.duration << "\n";

	DEBUG_ASSERT(animationMinTime == 0, "Probably need to implement support animations that do not start at 0");

	return result;
}

internal Skeleton
load_skeleton_glb(MemoryArena & memoryArena, GltfFile const & gltfFile, char const * modelName)
{
	auto const & jsonDocument 	= gltfFile.json;
	auto nodeArray = jsonDocument["nodes"].GetArray();
	
	JsonValue const * ptrModelNode = find_by_name(nodeArray, modelName);
	assert(ptrModelNode != nullptr);

	JsonValue const & modelNode = *ptrModelNode;

	u32 skinIndex 	= modelNode["skin"].GetInt();
	auto skin 		= jsonDocument["skins"][skinIndex].GetObject();
	auto jointArray = skin["joints"].GetArray();
	u32 jointCount 	= jointArray.Size();

	Skeleton result = { allocate_array<Bone>(memoryArena, jointCount, ALLOC_FILL_UNINITIALIZED) };
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

		if (node.HasMember("name"))
		{
			result.bones[i].name = node["name"].GetString();
 		}
		else
		{
			result.bones[i].name = "nameless bone";
		}
		

		if(node.HasMember("translation"))
		{
			auto translationArray = node["translation"].GetArray();
			result.bones[i].boneSpaceTransform.position.x = translationArray[0].GetFloat();
			result.bones[i].boneSpaceTransform.position.y = translationArray[1].GetFloat();
			result.bones[i].boneSpaceTransform.position.z = translationArray[2].GetFloat();
		}
		else
		{
			result.bones[i].boneSpaceTransform.position = {};
		}

		if (node.HasMember("rotation"))
		{
			auto rotationArray = node["rotation"].GetArray();
			result.bones[i].boneSpaceTransform.rotation.x = rotationArray[0].GetFloat();
			result.bones[i].boneSpaceTransform.rotation.y = rotationArray[1].GetFloat();
			result.bones[i].boneSpaceTransform.rotation.z = rotationArray[2].GetFloat();
			result.bones[i].boneSpaceTransform.rotation.w = rotationArray[3].GetFloat();
 
			result.bones[i].boneSpaceTransform.rotation = result.bones[i].boneSpaceTransform.rotation.inverse();
		}
		else
		{
			result.bones[i].boneSpaceTransform.rotation = quaternion::identity();
		}
	}

	int inverseBindMatrixAccessorIndex = skin["inverseBindMatrices"].GetInt();
	auto bindMatrixIterator = make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, inverseBindMatrixAccessorIndex);
	for(auto & bone : result.bones)
	{
		bone.inverseBindMatrix = get_and_advance<m44>(bindMatrixIterator);
	}

	return result;
}

internal MeshAsset
load_model_glb(MemoryArena & memoryArena, GltfFile const & gltfFile, char const * modelName)
{
	// auto rawBuffer 		= read_binary_file(memoryArena, filePath);
	// auto jsonBuffer 	= get_gltf_json_chunk(rawBuffer, memoryArena);
	// auto jsonString		= get_string(jsonBuffer);
	// auto jsonDocument	= parse_json(jsonString.c_str());
		
	auto & jsonDocument = gltfFile.json;

	auto nodeArray = jsonDocument["nodes"].GetArray();
	
	s32 positionAccessorIndex 	= -1; 
	s32 normalAccessorIndex 	= -1;
	s32 texcoordAccessorIndex 	= -1;

	s32 bonesAccessorIndex 			= -1;
	s32 boneWeightsAccessorIndex 	= -1;

	JsonValue const * ptrModelNode = find_by_name(nodeArray, modelName);
	assert(ptrModelNode != nullptr);
	JsonValue const & modelNode = *ptrModelNode;


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



	auto accessorArray 		= jsonDocument["accessors"].GetArray();
	auto bufferViewArray 	= jsonDocument["bufferViews"].GetArray();
	auto buffersArray 		= jsonDocument["buffers"].GetArray();
	
	// // Todo(Leo): what if there are multiple buffers in file? Where is the index? In "buffers"?
	// auto binaryBuffer 			= get_gltf_binary_chunk(memoryArena, rawBuffer, 0);

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

	s32 indexAccessorIndex 		= -1;
	indexAccessorIndex 			= primitivesArray[0].GetObject()["indices"].GetInt();
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

	auto vertices = allocate_array<Vertex>(memoryArena, vertexCount, ALLOC_FILL_UNINITIALIZED);

	auto normalIt 		= make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, normalAccessorIndex);
	auto posIt 			= make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, positionAccessorIndex);
	auto texcoordIt 	= make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, texcoordAccessorIndex);

	for (auto & vertex : vertices)
	{
		vertex.position = get_and_advance<v3>(posIt);
		vertex.normal 	= get_and_advance<v3>(normalIt);
		vertex.texCoord = get_and_advance<v2>(texcoordIt);
	}


	bool32 hasSkin = bonesAccessorIndex >= 0 && boneWeightsAccessorIndex >= 0;
	if (hasSkin)
	{
		auto bonesIterator = make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, bonesAccessorIndex);
		auto weightIterator = make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, boneWeightsAccessorIndex);

		for (auto & vertex : vertices)
		{
			vertex.boneIndices = get_and_advance<upoint4>(bonesIterator);
			vertex.boneWeights = get_and_advance<v4>(weightIterator);
			vertex.boneWeights = vertex.boneWeights.normalized();
		}
	}

	///// INDICES //////
	auto indexAccessor 	= accessorArray[indexAccessorIndex].GetObject();
	u64 indexCount 		= indexAccessor["count"].GetInt();
	auto indices 		= allocate_array<u16>(memoryArena, indexCount, ALLOC_FILL_UNINITIALIZED);

	auto indexIterator = make_gltf_iterator(gltfFile.binaryBuffer, jsonDocument, indexAccessorIndex);
	for (u16 & index : indices)
	{
		index = get_and_advance<u16>(indexIterator);
	}

	MeshAsset result = make_mesh_asset(std::move(vertices), std::move(indices));
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

	result.vertices = allocate_array<Vertex>(*memoryArena, indexCount, ALLOC_FILL_UNINITIALIZED);
	result.indices = allocate_array<u16>(*memoryArena, indexCount, ALLOC_FILL_UNINITIALIZED);

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
		result.vertices = allocate_array<Vertex>(*memoryArena, vertexCount, ALLOC_FILL_UNINITIALIZED);

		// 					  position 		normal   	color    	uv
		result.vertices[0] = {0, 0, 0, 		0, 0, 1, 	1, 1, 1, 	0, 0};
		result.vertices[1] = {1, 0, 0, 		0, 0, 1, 	1, 1, 1, 	1, 0};
		result.vertices[2] = {0, 1, 0, 		0, 0, 1, 	1, 1, 1, 	0, 1};
		result.vertices[3] = {1, 1, 0, 		0, 0, 1, 	1, 1, 1, 	1, 1};

		int indexCount = 6;

		if (flipIndices)
		{
			// Note(Leo): These seem to backwardsm but it because currently our gui is viewed from behind.
			result.indices = allocate_array<u16>(*memoryArena, {0, 2, 1, 1, 2, 3});
		}
		else
		{
			result.indices = allocate_array<u16>(*memoryArena, {0, 1, 2, 2, 1, 3});
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