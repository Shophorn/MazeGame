/*=============================================================================
Leo Tamminen

File things.
=============================================================================*/
#include <fstream>

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
				DEBUG_ASSERT(false, "Hello from bad choices department: 'Invalid glTF component type'");
		}
	}

	u32 get_component_count(char const * word)
	{
		if (cstring_equals(word, "SCALAR")) return 1;
		if (cstring_equals(word, "VEC2")) 	return 2;
		if (cstring_equals(word, "VEC3")) 	return 3;
		if (cstring_equals(word, "VEC4")) 	return 4;
		if (cstring_equals(word, "MAT2"))	return 4;
		if (cstring_equals(word, "MAT3")) 	return 9;
		if (cstring_equals(word, "MAT4")) 	return 16;

		assert(false);
		return -1;
	}
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

internal Array<byte>
read_binary_file(MemoryArena & memoryArena, const char * filename)
{
	auto file = std::ifstream (filename, std::ios::in|std::ios::binary);

	u64 size 		= get_ifstream_length(file);
	auto result 	= allocate_array<byte>(memoryArena, size);
	auto bufferPtr 	= reinterpret_cast<char *>(result.begin());

	file.read(bufferPtr, size);
	return result;
}


struct GltfFile
{
	Array<byte> 		memory;
	rapidjson::Document json;
	u64 				binaryChunkOffset;

	byte const * binary() const { return memory.data() + binaryChunkOffset; }
};


internal GltfFile
read_gltf_file(MemoryArena & memoryArena, char const * filename)
{
	auto memory = read_binary_file(memoryArena, filename);

	/* Note(Leo): We copy json textchunk out of buffer, because we need to append a null terminator
	in the end for rapidjson. We do not need to care for it later, rapidjson seems to do its thing 
	and then also not care. */
	Array<char> jsonChunk = {};
	
	u64 jsonChunkOffset	= glTF::headerLength;
	u32 jsonChunkLength = convert_bytes<u32>(memory.data(), jsonChunkOffset + glTF::chunkLengthPosition);
	
	{
		auto chunkType = convert_bytes<glTF::ChunkType>(memory.data(), jsonChunkOffset + glTF::chunkTypePosition);
		assert(chunkType == glTF::CHUNK_TYPE_JSON);
	}

	char const * start = reinterpret_cast<char const *>(memory.data()) + jsonChunkOffset + glTF::chunkInfoLength;
	char const * end = start + jsonChunkLength + 1;

	jsonChunk = allocate_array<char>(memoryArena, start, end);
	jsonChunk.last() = 0;

	// ----------------------------------------------------------------------------------------------------------------

	u64 binaryChunkOffset = glTF::headerLength + glTF::chunkInfoLength + jsonChunkLength;
	{
		auto chunkType = convert_bytes<glTF::ChunkType>(memory.data(), binaryChunkOffset + glTF::chunkTypePosition);
		assert(chunkType == glTF::CHUNK_TYPE_BINARY);
	}

	// ----------------------------------------------------------------------------------------------------------------

	GltfFile file = {};
	file.memory 			= std::move(memory);
	file.binaryChunkOffset 	= binaryChunkOffset + glTF::chunkInfoLength;

	file.json.Parse(jsonChunk.data());

	{
		/* Note(Leo): Clear this so we can be sute this is not used in meaningful way after this in rapidjson things, 
		and therefore we can just forget about it. */
		memset(jsonChunk.data(), 0, jsonChunk.count());
	}


	return file;
}