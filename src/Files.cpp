/*=============================================================================
Leo Tamminen

File things.
=============================================================================*/
#include <fstream>

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

namespace glTF
{
	// https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
	// https://raw.githubusercontent.com/KhronosGroup/glTF/master/specification/2.0/figures/gltfOverview-2.0.0b.png

	inline constexpr u32 magicNumber = 0x46546C67;


	inline constexpr s32 headerLength 			= 3 * sizeof(u32);
	inline constexpr s32 chunkInfoLength 		= 2 * sizeof(u32);  
	
	inline constexpr s32 chunkLengthPosition 	= 0;
	inline constexpr s32 chunkTypePosition 		= sizeof(u32);

	enum struct ChunkType : u32 { Json = 0x4e4f534a, Binary = 0x004e4942 };

	enum struct ComponentType : s32
	{
    	BYTE 			= 5120,
    	UNSIGNED_BYTE 	= 5121,
    	SHORT 			= 5122,
    	UNSIGNED_SHORT 	= 5123,
    	UNSIGNED_INT 	= 5125,
    	FLOAT 			= 5126,
	};

	u32 get_component_size (ComponentType type)
	{
		switch (type)
		{
			case ComponentType::BYTE:			return 1;
			case ComponentType::UNSIGNED_BYTE: 	return 1;
			case ComponentType::SHORT: 			return 2;
			case ComponentType::UNSIGNED_SHORT: return 2;
			case ComponentType::UNSIGNED_INT: 	return 4;
			case ComponentType::FLOAT: 			return 4;

			default:
				DEBUG_ASSERT(false, "Hello from bad choices department: 'Invalid glTF component type'");
		}
	}

	u32 get_component_count(char const * word)
	{
		if (strcmp(word, "SCALAR") == 0) return 1;
		if (strcmp(word, "VEC2") == 0) return 2;
		if (strcmp(word, "VEC3") == 0) return 3;
		if (strcmp(word, "VEC4") == 0) return 4;
		if (strcmp(word, "MAT2") == 0) return 4;
		if (strcmp(word, "MAT3") == 0) return 9;
		if (strcmp(word, "MAT4") == 0) return 16;

		assert(false);
		return -1;
	}
}

#if !defined internal
	THIS IS ERROR
#else
#undef internal

// Todo(Leo): Move this to platform side, so we do not need to link against anything in game code
#include <rapidjson/document.h>
#include <string>

#define internal static
#endif

using JsonDocument 	= rapidjson::Document;
using JsonValue 	= rapidjson::Value;

using BinaryBuffer 	= BETTERArray<byte>;
using TextBuffer 	= BETTERArray<char>;

template<typename T>
struct ArrayView
{
	BETTERArray<T> * array;
	u64 offset;
	u64 count;
	T * data() { return array->data + offset; }
};


template<typename TResult> TResult
get(BinaryBuffer const & buffer, u64 position)
{
	TResult result = *reinterpret_cast<TResult const *>(buffer.data() + position);
	return result;
}

// Todo(Leo): This is allocated uncontrollably, so we must do something about it
internal std::string
get_string(TextBuffer const & buffer)
{
	auto result = std::string(buffer.begin(), buffer.end());
	return result;	
}

internal TextBuffer
get_gltf_json_chunk(BinaryBuffer const & gltf, MemoryArena * memoryArena)
{
	using namespace glTF;

	u64 offset 		= headerLength;
	u32 chunkLength 	= get<u32>(gltf, offset + chunkLengthPosition);
	ChunkType chunkType = get<ChunkType>(gltf, offset + chunkTypePosition);

	while(chunkType != ChunkType::Json && offset < gltf.count())
	{
		offset 		+= chunkLength + chunkInfoLength;
		chunkLength  = get<u32>(gltf, offset + chunkLengthPosition);
		chunkType 	 = get<ChunkType>(gltf, offset + chunkTypePosition);
	}

	/* Note(Leo): Add this so we get to start of actual data of chunk
	instead the start of chunk info */
	u64 start = offset + chunkInfoLength;

	if (chunkType != ChunkType::Json)
	{
		return {};
	}


	char * memory = reinterpret_cast<char *>(allocate(*memoryArena, chunkLength));
	std::copy(gltf.data() + start, gltf.data() + start + chunkLength, memory);

	return TextBuffer(memory, chunkLength, chunkLength);








	// TextBuffer castResult 	= *reinterpret_cast<TextBuffer*>(&gltf);
	// TextBuffer result 		= copy_array_slice(memoryArena, castResult, start, chunkLength);


	// return result;
}

internal BinaryBuffer
get_gltf_binary_chunk(MemoryArena * memoryArena, BinaryBuffer const & gltf, s32 binaryChunkIndex = 0)
{
	using namespace glTF;

	/* Note(Leo): In glTF first chunk is json description, and following chunks
	are binary data buffers. We want to move to beginning of [binaryChunkIndex]th
	binary chunk*/

	u64 offset 		 = headerLength;
	u32 chunkLength 	 = get<u32>(gltf, offset + chunkLengthPosition);
	offset 				+= chunkInfoLength + chunkLength;

	for (int i = 0; i < binaryChunkIndex; ++i)
	{
		chunkLength  = get<u32>(gltf, offset + chunkLengthPosition);
		offset 		+= chunkInfoLength + chunkLength;
	}

	chunkLength  = get<u32>(gltf, offset + chunkLengthPosition);
	u64 start = offset + chunkInfoLength;
	// auto result  = copy_array_slice(memoryArena, gltf, start, chunkLength);

	byte * memory = reinterpret_cast<byte*>(allocate(*memoryArena, chunkLength));
	std::copy(gltf.data() + start, gltf.data() + start + chunkLength, memory);

	return BinaryBuffer(memory, chunkLength, chunkLength);

	// return result;
}

internal JsonDocument
parse_json(const char * jsonString)
{
	JsonDocument doc;
	doc.Parse(jsonString);
	return doc;
}

internal u64
get_ifstream_length(std::ifstream & stream)
{
	std::streampos current = stream.tellg();

	stream.seekg(0, std::ios::beg);
	std::streampos begin = stream.tellg();

	stream.seekg(0, std::ios::end);
	std::streampos end = stream.tellg();

	stream.seekg(current);
	
	u64 size = end - begin;
	return size;
}

internal BinaryBuffer
read_binary_file(MemoryArena * memoryArena, const char * path)
{
	auto file = std::ifstream (path, std::ios::in|std::ios::binary);
	// auto file = std::ifstream (	"w:/friendsimulator/models/ladder.glb",
	// 							std::ios::in|std::ios::binary);

	u64 size 	= get_ifstream_length(file);
	auto result 	= allocate_BETTER_array<byte>(*memoryArena, size);
	auto bufferPtr 	= reinterpret_cast<char *>(result.begin());

	file.read(bufferPtr, size);
	return result;
}


