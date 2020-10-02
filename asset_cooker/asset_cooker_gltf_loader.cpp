/*=============================================================================
Leo Tamminen

File things.
=============================================================================*/

static void mesh_generate_tangents(s32 vertexCount, Vertex * vertices, s32 indexCount, u16 * indices);



namespace glTF
{
	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
	// https://raw.githubusercontent.com/KhronosGroup/glTF/master/specification/2.0/figures/gltfOverview-2.0.0b.png

	inline constexpr u32 magicNumber = 0x46546C67;


	inline constexpr s32 headerLength 			= 3 * sizeof(u32);
	inline constexpr s32 chunkInfoLength 		= 2 * sizeof(u32);  
	
	inline constexpr s32 chunkLengthPosition 	= 0 * sizeof(u32);
	inline constexpr s32 chunkTypePosition 		= 1 * sizeof(u32);

	/* Note(Leo): These are as UINT32 values in raw binary file,
	as they denote what type the next chunk is. */
	enum ChunkType : u32
	{ 
		CHUNK_TYPE_JSON	 	= 0x4e4f534a,  	// ASCII: JSON
		CHUNK_TYPE_BINARY 	= 0x004e4942	// ASCII: BIN 
	};

	/* Note(Leo): These are stored as Json number literals in text format,
	so we can use s32 to represent these. */
	enum ComponentType : s32
	{
    	COMPONENT_TYPE_BYTE 			= 5120,
    	COMPONENT_TYPE_UNSIGNED_BYTE 	= 5121,
    	COMPONENT_TYPE_SHORT 			= 5122,
    	COMPONENT_TYPE_UNSIGNED_SHORT 	= 5123,
    	COMPONENT_TYPE_UNSIGNED_INT 	= 5125,
    	COMPONENT_TYPE_FLOAT 			= 5126,
	};

	u32 get_component_size (ComponentType type)
	{
		switch (type)
		{
			case COMPONENT_TYPE_BYTE:			return 1;
			case COMPONENT_TYPE_UNSIGNED_BYTE: 	return 1;
			case COMPONENT_TYPE_SHORT: 			return 2;
			case COMPONENT_TYPE_UNSIGNED_SHORT: return 2;
			case COMPONENT_TYPE_UNSIGNED_INT: 	return 4;
			case COMPONENT_TYPE_FLOAT: 			return 4;

			default:
				AssertMsg(false, "Hello from bad choices department: 'Invalid glTF component type'");
		}
	}

	/* Note(Leo): These are stored as string literals in gltf json chunk, 
	so we can just use those instead of separate enum. 

	Todo(Leo): We maybe could compare these as integers, since first 4 bytes
	are distinguishable.*/
	u32 get_component_count(char const * word)
	{
		if (cstring_equals(word, "SCALAR")) return 1;
		if (cstring_equals(word, "VEC2")) 	return 2;
		if (cstring_equals(word, "VEC3")) 	return 3;
		if (cstring_equals(word, "VEC4")) 	return 4;
		if (cstring_equals(word, "MAT2"))	return 4;
		if (cstring_equals(word, "MAT3")) 	return 9;
		if (cstring_equals(word, "MAT4")) 	return 16;

		Assert(false);
		return -1;
	}

	enum PrimitiveType : s32
	{
    	PRIMITIVE_TYPE_POINTS			= 0,
    	PRIMITIVE_TYPE_LINES 			= 1,
    	PRIMITIVE_TYPE_LINE_LOOP 		= 2,
    	PRIMITIVE_TYPE_LINE_STRIP 		= 3,
    	PRIMITIVE_TYPE_TRIANGLES 		= 4,
    	PRIMITIVE_TYPE_TRIANGLE_STRIP	= 5,
    	PRIMITIVE_TYPE_TRIANGLE_FAN 	= 6,
	};
}

/* Note(Leo): We have defined internal as static to denote
internal linkage in namespace lavel. Rapidjson however uses 
namespace internal in some places, so we undefine it for a
moment.

First we must check that internal is acutally defined, in case
we decide to remove it someday. Otherwise it would continue to
be defined here. Also we must re #define internal afterwards.

TODO(Leo): We probably should or this with all other external
libraries.
*/
#if !defined internal
	/* Note(Leo): We do not have internal defined, so something has changed,
	and our assumptions are not correct anymore */
	static_assert(false);
#else
#undef internal

// Todo(Leo): make proper asset baker with external library like assimp etc. and just use simple
// internal format here
#define RAPIDJSON_ASSERT(expr) Assert(expr)
#include <rapidjson/document.h>

#define internal static
#endif

using JsonDocument 	= rapidjson::Document;
using JsonValue 	= rapidjson::Value;
using JsonArray 	= rapidjson::GenericArray<true, JsonValue>;

// struct GltfFile
// {
// 	Array2<byte> 	memory;
// 	JsonDocument 	json;
// 	u64 			binaryChunkOffset;

// 	byte const * binary() const { return memory.memory + binaryChunkOffset; }
// };



struct glTFHeader
{
	u32 magic;
	u32 version;
	u32 length;
};

enum glTFChunkType : u32
{
	glTF_json_chunk 	= 0x4E4F534A, // ascii for JSON
	glTF_binary_chunk 	= 0x004E4942, // ascii for BIN
};

struct glTFChunk
{
	u32 			length;
	glTFChunkType 	type;
};

struct glTFFile
{
	JsonDocument 	json;	

	u64 			binaryChunkLength;
	u8 * 			binaryChunkMemory;
};


// internal u8 * read_binary_file(const char * filename, u64 * outSize)
// {
// 	// PlatformFileHandle file = platform_file_open(filename, FILE_MODE_READ);
// 	auto file = std::ifstream(filename, std::ios::in | std::ios::binary);

// 	Assert(file);

// 	file.seekg(0, std::ios::end);
// 	u64 filesize = file.tellg();
// 	file.seekg(0, std::ios::beg);


// 	u8 * result	= allocate<u8>(filesize);




// 	*outSize = filesize;
// 	return result;
// }

internal glTFFile
read_gltf_file(char const * filename)
{
	auto file = std::ifstream(filename, std::ios::in | std::ios::binary);

	glTFHeader header;
	file.read((char*)&header, sizeof(header));


	glTFChunk firstChunk;
	file.read((char*)&firstChunk, sizeof(firstChunk));

	assert(firstChunk.type == glTF_json_chunk);

	// Note(Leo): add 1 for null terminator for rapidjson
	u64 jsonChunkLength 	= firstChunk.length;
	char * jsonChunkMemory 	= allocate<char>(jsonChunkLength + 1);

	file.read(jsonChunkMemory, firstChunk.length);
	jsonChunkMemory[jsonChunkLength] = 0;

	glTFChunk secondChunk;
	file.read((char*)&secondChunk, sizeof(secondChunk));

	assert(secondChunk.type == glTF_binary_chunk);

	{
		u64 totalSize = sizeof(header) + sizeof(firstChunk) + firstChunk.length + sizeof(secondChunk) + secondChunk.length;
		assert(totalSize == header.length);
	}



	// printf("Reading gltf file from: %s", filename);
	
	// u64 memorySize;
	// auto memory = read_binary_file(filename, &memorySize);

	// /* Note(Leo): We copy json textchunk out of buffer, because we need to append a null terminator
	// in the end for rapidjson. We do not need to care for it later, rapidjson seems to do its thing 
	// and then also not care. */
	
	// u64 jsonChunkOffset	= glTF::headerLength;
	// u32 jsonChunkSize 	= memory_convert_bytes_to<u32>(memory, jsonChunkOffset + glTF::chunkLengthPosition);
	
	// {
	// 	auto chunkType = memory_convert_bytes_to<glTF::ChunkType>(memory.memory, jsonChunkOffset + glTF::chunkTypePosition);
	// 	Assert(chunkType == glTF::CHUNK_TYPE_JSON);
	// }

	// char const * start = reinterpret_cast<char const *>(memory.memory) + jsonChunkOffset + glTF::chunkInfoLength;
	// // char const * end = start + jsonChunkSize + 1;

	// Array2<char> jsonChunk = {jsonChunkSize + 1, 0, allocate<char>(jsonChunkSize + 1)};
	// array_2_fill_from_memory(jsonChunk, jsonChunkSize, start);
	// jsonChunk.push(0);

	// // ----------------------------------------------------------------------------------------------------------------

	// u64 binaryChunkOffset = glTF::headerLength + glTF::chunkInfoLength + jsonChunkSize;
	// {
	// 	auto chunkType = memory_convert_bytes_to<glTF::ChunkType>(memory.memory, binaryChunkOffset + glTF::chunkTypePosition);
	// 	Assert(chunkType == glTF::CHUNK_TYPE_BINARY);
	// }

	// ----------------------------------------------------------------------------------------------------------------

	// GltfFile file 			= {};
	// file.memory 			= memory;
	// file.binaryChunkOffset 	= binaryChunkOffset + glTF::chunkInfoLength;

	glTFFile result = {};
	result.json.Parse(jsonChunkMemory);

	result.binaryChunkLength = secondChunk.length;
	result.binaryChunkMemory = allocate<u8>(secondChunk.length);
	file.read((char*)result.binaryChunkMemory, result.binaryChunkLength);



	{
		/* Note(Leo): Clear this so we can be sure this is not used in meaningful way after this in rapidjson things, 
		and therefore we can just forget about it. */
		free(jsonChunkMemory);
		// memset(jsonChunk.memory, 0, jsonChunk.count);
	}

	// Note(Leo): Assert these once here, so we don't have to do that in every call hereafter.
	Assert(result.json.HasMember("nodes"));
	Assert(result.json.HasMember("accessors"));
	Assert(result.json.HasMember("bufferViews"));
	Assert(result.json.HasMember("buffers"));

	return result;
}

/*=============================================================================
Leo Tamminen

Mesh Loader

Todo(Leo):
	- use reference variables for rapidjson things
=============================================================================*/

template <typename T> internal T const * 
get_buffer_start(glTFFile const & file, u32 accessor)
{
	s32 viewIndex 	= file.json["accessors"][accessor]["bufferView"].GetInt();
	s32 offset 		= file.json["bufferViews"][viewIndex]["byteOffset"].GetInt();

	T const * start = reinterpret_cast<T const *> (file.binaryChunkMemory + offset);
	return start;
}

s32 find_bone_by_node(JsonArray & bones, int nodeIndex)
{
	for (int boneIndex = 0; boneIndex < bones.Size(); ++boneIndex)
	{
		if (nodeIndex == bones[boneIndex].GetInt())
			return boneIndex;
	}

	return -1;
}

s32 index_by_name(JsonArray & nodes, char const * name)
{
	for (s32 i = 0; i < nodes.Size(); ++i)
	{
		if (nodes[i].HasMember("name") && cstring_equals(nodes[i]["name"].GetString(), name))
		{
			return i;
		}
	}
	return -1;
}

#if 0
internal Animation
load_animation_glb(MemoryArena & allocator, GltfFile const & file, char const * animationName)
{
	AssertMsg(file.json.HasMember("animations"), "No Animations found");

	auto animArray 		= file.json["animations"].GetArray();
	s32 animationIndex 	= index_by_name(animArray, animationName);

	if(animationIndex < 0)
	{
		s32 logMessageCapacity = 2048;
		String logMessage = push_temp_string(logMessageCapacity);
		string_append_format(logMessage, logMessageCapacity, "Animation not found. Requested: ", animationName, ", available: \n");

		// auto log = log_debug();
		// log << "Animation not found. Requested: " << animationName << ", available:\n";
		for (auto const & anim : animArray)
		{
			string_append_format(logMessage, logMessageCapacity, "\t", anim["name"].GetString(), "\n");
			// log << "\t" << anim["name"].GetString() << "\n";
		}
	}
	Assert(animationIndex >= 0);
	JsonValue const & namedAnimation = animArray[animationIndex];

	auto jsonChannels 	= animArray[animationIndex]["channels"].GetArray();
	auto samplers 		= animArray[animationIndex]["samplers"].GetArray();

	auto accessors 		= file.json["accessors"].GetArray();

	s32 translationChannelCount = 0;
	s32 rotationChannelCount = 0;

	for (auto const & jsonChannel : jsonChannels)
	{
		char const * path 	= jsonChannel["target"]["path"].GetString();

		if (cstring_equals(path, "translation"))	{ translationChannelCount += 1; }
		else if(cstring_equals(path, "rotation")) { rotationChannelCount += 1; }
		else if (cstring_equals(path, "scale"))	 {}
		else
			Assert(false);
	}

	Animation result = {};
	result.translationChannels 	= push_array_2<TranslationChannel>(allocator, translationChannelCount, ALLOC_ZERO_MEMORY);
	result.rotationChannels 	= push_array_2<RotationChannel>(allocator, rotationChannelCount, ALLOC_ZERO_MEMORY);

	float minTime = highest_f32;
	float maxTime = lowest_f32;

	for (auto const & jsonChannel : jsonChannels)
	{
		/* Note(Leo): input refers to keyframe times and output to actual values,
		i.e. the input and output of interpolation function for animation. */
		int samplerIndex 	= jsonChannel["sampler"].GetInt();
		int inputAccessor 	= samplers[samplerIndex]["input"].GetInt();
		int outputAccessor 	= samplers[samplerIndex]["output"].GetInt();

		minTime = min_f32(minTime, accessors[inputAccessor]["min"][0].GetFloat());
		maxTime = max_f32(maxTime, accessors[inputAccessor]["max"][0].GetFloat());


		// ------------------------------------------------------------------------------

		enum { 	ANIMATION_CHANNEL_TRANSLATION,
				ANIMATION_CHANNEL_ROTATION,
				ANIMATION_CHANNEL_SCALE,
		} channelType;

		InterpolationMode 	interpolationMode;
		{
			char const * interpolationModeStr = samplers[samplerIndex]["interpolation"].GetString();

			if(cstring_equals(interpolationModeStr, "STEP"))				interpolationMode = INTERPOLATION_MODE_STEP;
			else if (cstring_equals(interpolationModeStr, "LINEAR"))		interpolationMode = INTERPOLATION_MODE_LINEAR;
			else if (cstring_equals(interpolationModeStr,"CUBICSPLINE"))	interpolationMode = INTERPOLATION_MODE_CUBICSPLINE;
			else
				Assert(false);
		
			char const * path 	= jsonChannel["target"]["path"].GetString();

			if (cstring_equals(path, "translation"))	channelType = ANIMATION_CHANNEL_TRANSLATION;
			else if(cstring_equals(path, "rotation"))	channelType = ANIMATION_CHANNEL_ROTATION;
			else if (cstring_equals(path, "scale"))		channelType = ANIMATION_CHANNEL_SCALE;
			else
				Assert(false);
		}

		// ------------------------------------------------------------------------------

		bool32 supportedChannel = 	(channelType == ANIMATION_CHANNEL_TRANSLATION)
									|| (channelType == ANIMATION_CHANNEL_ROTATION);


		if (supportedChannel == false)
		{
			continue;
		}

		s32 targetIndex = -1;
		{
			/* Note(Leo): Bones (or joints) are somewhat cumbersomely behind different list, so we need to remap those.
			Probably the best idea would be to have skeleton reference here too. */
			Assert(file.json.HasMember("skins") && file.json["skins"].Size() == 1 && file.json["skins"][0].HasMember("joints"));

			auto bonesArray 	= file.json["skins"][0]["joints"].GetArray();
			s32 targetNode 		= jsonChannel["target"]["node"].GetInt();
			targetIndex = find_bone_by_node(bonesArray, targetNode);
		
			Assert(targetIndex >= 0);
		}
		
		// ------------------------------------------------------------------------------

		int keyframeCount = accessors[inputAccessor]["count"].GetInt();

		float const * timesStart = get_buffer_start<float>(file, inputAccessor);
		// float const * timesEnd 	= timesStart + keyframeCount;
		Array2<f32> times = push_array_2<float>(allocator, keyframeCount, ALLOC_GARBAGE);
		array_2_fill_from_memory(times, keyframeCount, timesStart);

		/* Note(Leo): CUBICSPLINE interpolation has three values in total: inTangent,
		splineVertex(aka actual keyframe value) and outTangent. */
		s32 valueCount = interpolationMode == INTERPOLATION_MODE_CUBICSPLINE
						? keyframeCount * 3
						: keyframeCount;

		switch(channelType)
		{
			case ANIMATION_CHANNEL_TRANSLATION:
			{
				v3 const * start 		= get_buffer_start<v3>(file, outputAccessor);
				v3 const * end 			= start + valueCount;

				TranslationChannel channel 	= {};
				channel.times 				= times;

				// Note(Leo): We do not yet support cubicspline interpolation, so we do not need the data for it, convert to linear				
				if (interpolationMode == INTERPOLATION_MODE_CUBICSPLINE)
				{
					interpolationMode = INTERPOLATION_MODE_LINEAR;
					
					channel.translations = push_array_2<v3>(allocator, keyframeCount, ALLOC_GARBAGE);
					array_2_fill_uninitialized(channel.translations);

					for (s32 keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
					{
						s32 valueIndex = keyframeIndex * 3 + 1;
						channel.translations[keyframeIndex] = start[valueIndex];
					}					
				}
				else
				{
					channel.translations = push_array_2<v3>(allocator, keyframeCount, ALLOC_GARBAGE);
					array_2_fill_from_memory(channel.translations, keyframeCount, start);

				}
			  	
			  	channel.interpolationMode = interpolationMode;
				channel.targetIndex 		= targetIndex;

				result.translationChannels.push(channel);
			} break;

			case ANIMATION_CHANNEL_ROTATION:
			{
				quaternion const * start 	= get_buffer_start<quaternion>(file, outputAccessor);
				quaternion const * end 		= start + valueCount;

				RotationChannel channel 	= {};
				channel.times 				= std::move(times);

				// Note(Leo): We do not yet support cubicspline interpolation, so we do not need the data for it, convert to linear				
				if (interpolationMode == INTERPOLATION_MODE_CUBICSPLINE)
				{
					interpolationMode = INTERPOLATION_MODE_LINEAR;
					
					channel.rotations = push_array_2<quaternion>(allocator, keyframeCount, ALLOC_GARBAGE);
					array_2_fill_uninitialized(channel.rotations);

					for (s32 keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
					{
						s32 valueIndex = keyframeIndex * 3 + 1;
						channel.rotations[keyframeIndex] = start[valueIndex];
					}
				}
				else
				{
					channel.rotations = push_array_2<quaternion>(allocator, keyframeCount, ALLOC_GARBAGE);
					array_2_fill_from_memory(channel.rotations, keyframeCount, start);
				}
				
				channel.interpolationMode 	= interpolationMode;
				channel.targetIndex 		= targetIndex;


				/* Note(Leo): For some reason, we get inverted rotations from blender produced gltf-files,
				so we need to invert them here. I have not tested glTF files from other sources.

				If interpolation mode is CUBICSPLINE, we also have tangent quaternions, that are not
				unit length, so we must use proper inversion method for those. */
				for (auto & rotation : channel.rotations)
				{
					rotation = inverse_non_unit_quaternion(rotation);
				}

				result.rotationChannels.push(channel);
			} break;

			default:
				log_debug(FILE_ADDRESS, "Invalid or unimplemented animation channel: '", channelType, "' for bone ", targetIndex);
				continue;

		}
	}
	result.duration = maxTime - minTime;

	log_animation(1, "Animation loaded, duration: ", result.duration);

	AssertMsg(minTime == 0, "Probably need to implement support animations that do not start at 0");

	return result;
}
#endif

#if 0
internal AnimatedSkeleton
load_skeleton_glb(MemoryArena & allocator, GltfFile const & file, char const * modelName)
{
	auto nodes = file.json["nodes"].GetArray();

	s32 nodeIndex = index_by_name(nodes, modelName);
	Assert(nodeIndex >= 0);

	Assert(nodes[nodeIndex].HasMember("skin"));
	Assert(file.json.HasMember("skins"));

	// Note(Leo): We currently support only 1 skin per file, so that we can easily load correct animation.
	Assert(file.json["skins"].Size() == 1);
	Assert(file.json["skins"][0].HasMember("joints"));

	auto skin 		= file.json["skins"][0].GetObject();
	auto jsonBones 	= skin["joints"].GetArray();
	u32 boneCount 	= jsonBones.Size();

	m44 const * inverseBindMatrices = get_buffer_start<m44>(file, skin["inverseBindMatrices"].GetInt());

	AnimatedSkeleton skeleton = {};
	skeleton.bones = push_array_2<AnimatedBone>(allocator, boneCount, ALLOC_ZERO_MEMORY);
	
	for (int boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		u32 nodeIndex 		= jsonBones[boneIndex].GetInt();
		auto const & node 	= nodes[nodeIndex].GetObject();

		s32	 parent = -1;

		for (int parentBoneIndex = 0; parentBoneIndex < boneCount; ++parentBoneIndex)
		{
			u32 parentNodeIndex 	= jsonBones[parentBoneIndex].GetInt();
			auto const & parentNode = nodes[parentNodeIndex].GetObject();

			if (parentNode.HasMember("children"))
			{
				auto childArray = parentNode["children"].GetArray();
				for(auto const & childNodeIndex : childArray)
				{
					u32 childBoneIndex = find_bone_by_node(jsonBones, childNodeIndex.GetInt()); 
					if (childBoneIndex == boneIndex)
					{
						parent = parentBoneIndex;
					}
				}
			}
		}

		if (parent < 0)
		{
			/* Note(Leo): Only accept one bone with no parent, and we recognize it by name now

			This is a hack, but it works well enough. Bones that are not root,
			but have no parent are likely to be ik rig bones or similar non deforming bones. 
			We have made notion to asset workflow to remove these bones before exporting,
			this will catch those if forgotten. */
			Assert(node.HasMember("name"));
			if (cstring_equals("Root", node["name"].GetString()) == false)
			{
				parent = 0;
			}
		}

		auto boneSpaceTransform = identity_transform;
		if(node.HasMember("translation"))
		{
			auto translationArray = node["translation"].GetArray();
			boneSpaceTransform.position.x = translationArray[0].GetFloat();
			boneSpaceTransform.position.y = translationArray[1].GetFloat();
			boneSpaceTransform.position.z = translationArray[2].GetFloat();
		}

		if (node.HasMember("rotation"))
		{
			auto rotationArray = node["rotation"].GetArray();
			boneSpaceTransform.rotation.x = rotationArray[0].GetFloat();
			boneSpaceTransform.rotation.y = rotationArray[1].GetFloat();
			boneSpaceTransform.rotation.z = rotationArray[2].GetFloat();
			boneSpaceTransform.rotation.w = rotationArray[3].GetFloat();
 
			boneSpaceTransform.rotation = inverse_quaternion(boneSpaceTransform.rotation);
		}

		m44 inverseBindMatrix = inverseBindMatrices[boneIndex];
		skeleton.bones.push(make_bone(boneSpaceTransform, inverseBindMatrix, parent));
	}

	/* Todo(Leo): Check that parent always comes before bone itself. Currently we have no mechanism
	to fix the situation, so we just abort.*/
	Assert(skeleton.bones[0].parent < 0);

	for (s32 i = 1; i < skeleton.bones.count; ++i)
	{
		Assert((skeleton.bones[i].parent < i) && skeleton.bones[i].parent >= 0);
	}

	return skeleton;
}
#endif

#if 1
internal MeshAssetData
// load_mesh_glb(GltfFile const & file, char const * modelName)
asset_cooker_load_mesh_glb(char const * gltfFilename, char const * modelName)
{
	glTFFile const & file = read_gltf_file(gltfFilename);

	auto nodes = file.json["nodes"].GetArray();
	
	s32 nodeIndex = index_by_name(nodes, modelName);
	// Assert(nodeIndex >= 0);

	if (nodeIndex < 0)
	{
		// log_debug(FILE_ADDRESS, "Asset not found! modelName = ", modelName);
		printf("%s:%d: Asset not found: %s \n", __FILE__, __LINE__, modelName);
		Assert(false);
	}

	Assert(file.json.HasMember("meshes"));
	Assert(nodes[nodeIndex].HasMember("mesh"));

	u32 meshIndex 	= nodes[nodeIndex]["mesh"].GetInt();
	auto meshObject	= file.json["meshes"][meshIndex].GetObject();

	/* Note(Leo): glTF primitives may contain following fields:
		- attributes,
		- indices,
		- material,
		- mode,
		- targets,
		- extensions,
		- extras

		We only care about attributes, indices and mode,
		Currently we support only single primitive, since we do not have any submesh systems.
	*/

	Assert (meshObject["primitives"].Size() == 1);

	auto meshPrimitive = meshObject["primitives"][0].GetObject();

	Assert (meshPrimitive.HasMember("attributes"));
	Assert (meshPrimitive.HasMember("indices"));
	Assert (meshPrimitive.HasMember("mode") == false || meshPrimitive["mode"].GetInt() == glTF::PRIMITIVE_TYPE_TRIANGLES);

	auto attribObject		= meshPrimitive["attributes"].GetObject();

	s32 positionAccessor 	= attribObject["POSITION"].GetInt();
	s32 normalAccessor 		= attribObject["NORMAL"].GetInt();
	s32 texcoordAccessor 	= attribObject["TEXCOORD_0"].GetInt();

	s32 tangentAccessor = -1;
	if (attribObject.HasMember("TANGENT"))
	{
		tangentAccessor = attribObject["TANGENT"].GetInt();		
	}

	JsonArray const & accessors = file.json["accessors"].GetArray();

	u32 positionCount 	= accessors[positionAccessor]["count"].GetInt();
	u32 normalCount 	= accessors[normalAccessor]["count"].GetInt();
	u32 texcoordCount 	= accessors[texcoordAccessor]["count"].GetInt();

	Assert(positionCount == normalCount && positionCount == texcoordCount);

	u32 vertexCount = positionCount;

	// -----------------------------------------------------------------------------

	// Vertex * vertices 			= push_memory<Vertex>(allocator, vertexCount, ALLOC_GARBAGE);
	Vertex * vertices = allocate<Vertex>(vertexCount);
	VertexSkinData * skinning 	= nullptr;

	v3 const * positions 	= get_buffer_start<v3>(file, positionAccessor);
	v3 const * normals 		= get_buffer_start<v3>(file, normalAccessor);
	v2 const * texcoords 	= get_buffer_start<v2>(file, texcoordAccessor);

	Assert(vertexCount < max_value_u16 && "We need more indices!");

	for (u16 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
	{
		vertices[vertexIndex].position 	= positions[vertexIndex];
		vertices[vertexIndex].normal 	= normals[vertexIndex];
		vertices[vertexIndex].texCoord 	= texcoords[vertexIndex];
	}

	bool hasSkin = attribObject.HasMember("JOINTS_0") && attribObject.HasMember("WEIGHTS_0");

	if (hasSkin)
	{
		// skinning = push_memory<VertexSkinData>(allocator, vertexCount, ALLOC_GARBAGE);
		skinning = allocate<VertexSkinData>(vertexCount);

		s32 bonesAccessorIndex 		= attribObject["JOINTS_0"].GetInt();
		s32 boneWeightsAccessorIndex = attribObject["WEIGHTS_0"].GetInt();

		// Note(Leo): only UNSIGNED_SHORT supported currently
		auto componentType = (glTF::ComponentType)accessors[bonesAccessorIndex]["componentType"].GetInt();
		Assert(componentType == glTF::COMPONENT_TYPE_UNSIGNED_SHORT);

		u16 const * bones = get_buffer_start<u16>(file, bonesAccessorIndex);
		f32 const * weights = get_buffer_start<f32>(file, boneWeightsAccessorIndex);

		for (u16 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
		{
			// Note(Leo): joints are stored as UINT16 in file, but we use UINT32, hence the intermediary buffer
			u16 bonesFromBuffer [4];
			memory_copy(bonesFromBuffer, bones + (vertexIndex * 4), sizeof(bonesFromBuffer));
			for(s32 boneIndexIndex = 0; boneIndexIndex < 4; ++boneIndexIndex)
			{
				skinning[vertexIndex].boneIndices[boneIndexIndex] = static_cast<u32>(bonesFromBuffer[boneIndexIndex]);
			}

			memory_copy(&skinning[vertexIndex].boneWeights, weights + (vertexIndex * 4), sizeof(skinning[vertexIndex].boneWeights));
		} 

	}

	// ----------------------------------------------------------------------------------

	s32 indexAccessor 	= meshPrimitive["indices"].GetInt();
	s32 indexCount 		= accessors[indexAccessor]["count"].GetInt();

	u16 const * indexStart 	= get_buffer_start<u16>(file, indexAccessor);
	// u16 * indices 			= push_and_copy_memory(allocator, indexCount, indexStart);
	u16 * indices = allocate<u16>(indexCount);
	memcpy(indices, indexStart, indexCount * sizeof(u16));

	// ----------------------------------------------------------------------------------

	MeshAssetData result 	= {};
	result.vertexCount		= vertexCount;
	result.vertices 		= vertices;
	result.skinning 		= skinning;

	result.indexCount 	= indexCount;
	result.indices 		= indices;

	mesh_generate_tangents(result.vertexCount, result.vertices, result.indexCount, result.indices);

	free(file.binaryChunkMemory);

	return result;
}
#endif


static void mesh_generate_tangents(s32 vertexCount, Vertex * vertices, s32 indexCount, u16 * indices)
{
	s64 triangleCount = indexCount / 3;

	v3 * vertexTangents = allocate<v3>(vertexCount);
	
	for(s64 i = 0; i < triangleCount; ++i)
	{
		u16 index0 = indices[i * 3];
		u16 index1 = indices[i * 3 + 1];
		u16 index2 = indices[i * 3 + 2];

		v3 p0 = vertices[index0].position;
		v2 uv0 = vertices[index0].texCoord;

		v3 p1 = vertices[index1].position;
		v2 uv1 = vertices[index1].texCoord;
		
		v3 p2 = vertices[index2].position;
		v2 uv2 = vertices[index2].texCoord;

		v3 p01 = p1 - p0;
		v3 p02 = p2 - p0;

		v2 uv01 = uv1 - uv0;
		v2 uv02 = uv2 - uv0;

		// Note(Leo): Why is this called r? ThinMatrix has link to explanation in his normal map video
		// Note(Leo): Also this looks suspiciously like cross_2d, and someone named that "det", determinant?
		f32 r = 1.0f / (uv01.x * uv02.y - uv01.y * uv02.x);

		v3 tangent = (p01 * uv02.y - p02 * uv01.y) * r;
		tangent = normalize_v3(tangent);

		vertexTangents[index0] += tangent;
		vertexTangents[index1] += tangent;
		vertexTangents[index2] += tangent;
	}

	for (u32 i = 0; i < vertexCount; ++i)
	{
		vertices[i].tangent = normalize_v3(vertexTangents[i]);
	}

	free(vertexTangents);
}