/*=============================================================================
Leo Tamminen

File things.
=============================================================================*/

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

internal Array2<byte>
read_binary_file(MemoryArena & allocator, const char * filename)
{
	PlatformFileHandle file = platform_file_open(filename, FILE_MODE_READ);

	Assert(file != nullptr);

	s64 size 				= platform_file_get_size(file);
	auto result 			= push_array_2<byte>(allocator, size, ALLOC_GARBAGE);
	auto bufferPtr 			= reinterpret_cast<char *>(result.begin());

	platform_file_read(file, size, bufferPtr);
	
	platform_file_close(file);
	return result;
}

struct GltfFile
{
	Array2<byte> 	memory;
	JsonDocument 	json;
	u64 			binaryChunkOffset;

	byte const * binary() const { return memory.memory + binaryChunkOffset; }
};


internal GltfFile
read_gltf_file(MemoryArena & allocator, char const * filename)
{
	log_asset(2, "Reading gltf file from: ", filename);
	
	auto memory = read_binary_file(allocator, filename);

	/* Note(Leo): We copy json textchunk out of buffer, because we need to append a null terminator
	in the end for rapidjson. We do not need to care for it later, rapidjson seems to do its thing 
	and then also not care. */
	
	u64 jsonChunkOffset	= glTF::headerLength;
	u32 jsonChunkSize 	= memory_convert_bytes_to<u32>(memory.memory, jsonChunkOffset + glTF::chunkLengthPosition);
	
	{
		auto chunkType = memory_convert_bytes_to<glTF::ChunkType>(memory.memory, jsonChunkOffset + glTF::chunkTypePosition);
		Assert(chunkType == glTF::CHUNK_TYPE_JSON);
	}

	char const * start = reinterpret_cast<char const *>(memory.memory) + jsonChunkOffset + glTF::chunkInfoLength;
	// char const * end = start + jsonChunkSize + 1;

	Array2<char> jsonChunk = push_array_2<char>(allocator, jsonChunkSize + 1, ALLOC_ZERO_MEMORY);
	array_2_fill_from_memory(jsonChunk, jsonChunkSize, start);
	jsonChunk.push(0);

	// ----------------------------------------------------------------------------------------------------------------

	u64 binaryChunkOffset = glTF::headerLength + glTF::chunkInfoLength + jsonChunkSize;
	{
		auto chunkType = memory_convert_bytes_to<glTF::ChunkType>(memory.memory, binaryChunkOffset + glTF::chunkTypePosition);
		Assert(chunkType == glTF::CHUNK_TYPE_BINARY);
	}

	// ----------------------------------------------------------------------------------------------------------------

	GltfFile file 			= {};
	file.memory 			= memory;
	file.binaryChunkOffset 	= binaryChunkOffset + glTF::chunkInfoLength;

	file.json.Parse(jsonChunk.memory);

	{
		/* Note(Leo): Clear this so we can be sure this is not used in meaningful way after this in rapidjson things, 
		and therefore we can just forget about it. */
		memset(jsonChunk.memory, 0, jsonChunk.count);
	}

	// Note(Leo): Assert these once here, so we don't have to do that in every call hereafter.
	Assert(file.json.HasMember("nodes"));
	Assert(file.json.HasMember("accessors"));
	Assert(file.json.HasMember("bufferViews"));
	Assert(file.json.HasMember("buffers"));

	return file;
}