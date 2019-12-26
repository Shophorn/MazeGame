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

	inline constexpr uint32 magicNumber = 0x46546C67;


	inline constexpr int32 headerLength 			= 3 * sizeof(uint32);
	inline constexpr int32 chunkInfoLength 			= 2 * sizeof(uint32);  
	
	inline constexpr int32 chunkLengthPosition 		= 0;
	inline constexpr int32 chunkTypePosition 		= sizeof(uint32);

	enum struct ChunkType : uint32 { Json = 0x4e4f534a, Binary = 0x004e4942 };

	enum struct ComponentType : int32
	{
    	BYTE 			= 5120,
    	UNSIGNED_BYTE 	= 5121,
    	SHORT 			= 5122,
    	UNSIGNED_SHORT 	= 5123,
    	UNSIGNED_INT 	= 5125,
    	FLOAT 			= 5126,
	};

	uint64 get_default_stride (ComponentType type)
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
				MAZEGAME_ASSERT(false, "Hello bad choices: Invalid glTF component type");
		}
	}
}

#if !defined internal
	THIS IS ERROR
#else
#undef internal

#include <rapidjson/document.h>
#include <string>

#define internal static
#endif

using JsonDocument 	= rapidjson::Document;
using JsonValue 	= rapidjson::Value;

using BinaryBuffer 	= ArenaArray<byte>;
using TextBuffer 	= ArenaArray<char>;

template<typename T>
struct ArrayView
{
	ArenaArray<T> * array;
	uint64 offset;
	uint64 count;
	T * data() { return array->data + offset; }
};


template<typename TResult> TResult
get(BinaryBuffer * buffer, uint64 position)
{
	TResult result = *reinterpret_cast<TResult*>(buffer->data + position);
	return result;
}

internal std::string
get_string(TextBuffer * buffer)
{
	auto result = std::string(buffer->begin(), buffer->end());
	return result;	
}

internal TextBuffer
get_gltf_json_chunk(BinaryBuffer * gltf, MemoryArena * memoryArena)
{
	using namespace glTF;

	uint64 offset 		= headerLength;
	uint32 chunkLength 	= get<uint32>(gltf, offset + chunkLengthPosition);
	ChunkType chunkType = get<ChunkType>(gltf, offset + chunkTypePosition);

	while(chunkType != ChunkType::Json && offset < gltf->count)
	{
		offset 		+= chunkLength + chunkInfoLength;
		chunkLength  = get<uint32>(gltf, offset + chunkLengthPosition);
		chunkType 	 = get<ChunkType>(gltf, offset + chunkTypePosition);
	}

	/* Note(Leo): Add this so we get to start of actual data of chunk
	instead the start of chunk info */
	uint64 start = offset + chunkInfoLength;

	if (chunkType != ChunkType::Json)
	{
		return {};
	}

	auto castResult = cast_array_type<char>(gltf);
	auto result 	= copy_array_slice(memoryArena, &castResult, start, chunkLength);


	return result;
}

internal BinaryBuffer
get_gltf_binary_chunk(MemoryArena * memoryArena, BinaryBuffer * gltf, int32 binaryChunkIndex = 0)
{
	using namespace glTF;

	/* Note(Leo): In glTF first chunk is json description, and following chunks
	are binary data buffers. We want to move to beginning of [binaryChunkIndex]th
	binary chunk*/

	uint64 offset 		 = headerLength;
	uint32 chunkLength 	 = get<uint32>(gltf, offset + chunkLengthPosition);
	offset 				+= chunkInfoLength + chunkLength;

	for (int i = 0; i < binaryChunkIndex; ++i)
	{
		chunkLength  = get<uint32>(gltf, offset + chunkLengthPosition);
		offset 		+= chunkInfoLength + chunkLength;
	}

	chunkLength  = get<uint32>(gltf, offset + chunkLengthPosition);
	uint64 start = offset + chunkInfoLength;
	auto result  = copy_array_slice(memoryArena, gltf, start, chunkLength);

	return result;
}

internal JsonDocument
parse_json(const char * jsonString)
{
	JsonDocument doc;
	doc.Parse(jsonString);
	return doc;
}

internal uint64
get_ifstream_length(std::ifstream & stream)
{
	std::streampos current = stream.tellg();

	stream.seekg(0, std::ios::beg);
	std::streampos begin = stream.tellg();

	stream.seekg(0, std::ios::end);
	std::streampos end = stream.tellg();

	stream.seekg(current);
	
	uint64 size = end - begin;
	return size;
}

internal BinaryBuffer
read_binary_file(MemoryArena * memoryArena, const char * path)
{
	auto file = std::ifstream (path, std::ios::in|std::ios::binary);
	// auto file = std::ifstream (	"w:/friendsimulator/models/ladder.glb",
	// 							std::ios::in|std::ios::binary);

	uint64 size 	= get_ifstream_length(file);
	auto result 	= push_array<byte>(memoryArena, size);
	auto bufferPtr 	= reinterpret_cast<char *>(result.begin());

	file.read(bufferPtr, size);
	return result;
}


