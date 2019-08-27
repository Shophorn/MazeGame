/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>


#if MAZEGAME_DEVELOPMENT
	#if MAZEGAME_INCLUDE_STD_IOSTREAM
	void PrintAssert(const char * file, int line, const char * message, const char * expression)
	{
		std::cout
			<< "Assertion failed [" << file << ":" << line << "]: \""
			<< message << "\", expression (" << expression << ")\n";
	}
	#endif

	#define MAZEGAME_ASSERT(expr, msg) if (!(expr)) { PrintAssert(__FILE__, __LINE__, msg, #expr); abort(); }
#endif

#include "MazegameEssentials.hpp"
#include "Array.hpp"

#include "Math.cpp"
#include "Vectors.cpp"
#include "Matrices.cpp"
#include "Quaternion.cpp"

// #include <vector>

struct MemoryArena
{
	byte * 	memory;
	uint64 	size;
	uint64 	used;

	uint64 Available () { return size - used; }
};

internal MemoryArena
CreateMemoryArena(byte * memory, uint64 size)
{
	MemoryArena resultArena = {};
	resultArena.memory = memory;
	resultArena.size = size;
	resultArena.used = 0;
	return resultArena;
}

internal void
FlushArena(MemoryArena * arena)
{
	arena->used = 0;
}

template<typename Type>
struct ArenaArray
{
	Type * memory;
	uint64 count;

	Type & operator [] (int index)
	{
		{
			MAZEGAME_ASSERT(count > 0, "Array count is zero");

			char wordBuffer [200];
			sprintf(wordBuffer,"Index (%d) is more than count (%d)", index, count);
			MAZEGAME_ASSERT (index < count, wordBuffer);
		}

		return memory [index];
	}
};

template<typename Type>
ArenaArray<Type> PushArray(MemoryArena * arena, uint64 count)
{
	uint64 requiredSize = sizeof(Type) * count;

	if (requiredSize <= arena->Available())
	{
		// Todo[Memory](Leo): Alignment
		MAZEGAME_NO_INIT ArenaArray<Type> resultArray;
		resultArray.memory = reinterpret_cast<Type *>(arena->memory + arena->used);
		resultArray.count = count;

		arena->used += requiredSize;

		return resultArray;
	}
	else
	{
		MAZEGAME_ASSERT(false, "Not enough memory available in arena");
	}
}

struct Handle
{
	uint64 index = -1;
	operator uint64 () { return index; }
};


struct Vertex
{
	Vector3 position;
	Vector3 normal;
	Vector3 color;
	Vector2 texCoord;
};

enum struct IndexType : uint32 { UInt16, UInt32 };

struct Mesh
{
	ArenaArray<Vertex> vertices;
	ArenaArray<uint16> indices;

	IndexType indexType;

	void * indexData;
};

struct MeshHandle : Handle {};

struct TextureAsset
{
	ArenaArray<uint32> pixels;

	int32 	width;
	int32 	height;
	int32 	channels;
};

struct TextureHandle : Handle {};

struct GameMaterial
{
    // Note(Leo): Ignore now, later we use this to differentiate different material layouts
    int32 materialType;

    TextureHandle albedo;
    TextureHandle metallic;
};

struct MaterialHandle : Handle {};


struct RenderedObjectHandle : Handle {};

namespace platform
{
	/* Note(Leo): This seems good, but for some reasoen I feel
	unease about using even this kind of simple inheritance */
	struct IGraphicsContext
	{
		virtual MeshHandle 		PushMesh(Mesh * mesh) 					= 0;
		virtual TextureHandle 	PushTexture (TextureAsset * asset) 		= 0;
		virtual MaterialHandle 	PushMaterial (GameMaterial * material) 	= 0;
		
		virtual RenderedObjectHandle PushRenderedObject(MeshHandle mesh, MaterialHandle material) = 0;

		virtual void Apply() = 0;
	};
}

namespace game
{
	enum struct InputButtonState
	{
		/* Note(Leo): These specific values have been selected so that it 
		is easy to compute value with simple addition based on input values*/
		IsUp 		= 0,
		WentDown 	= 1,
		WentUp 		= 2,
		IsDown 		= 3
	};

	struct Input
	{
		Vector2 move;
		Vector2 look;

		InputButtonState jump;

		bool32 zoomIn;
		bool32 zoomOut;

		real32 timeDelta;
	};

	struct PlatformInfo
	{
		platform::IGraphicsContext * graphicsContext;

		int32 windowWidth;
		int32 windowHeight;
	};
	
	struct Memory
	{
		bool32 isInitialized;

		void * persistentMemory;
		uint64 persistentMemorySize;

		void * transientMemory;
		uint64 transientMemorySize;
	};
	
	struct RenderInfo
	{
		/* Todo(Leo): Make a proper container for these
		It should be such that items are stored as per vulkan physical device's
		min uniformbuffer alignment, so we can just map the whole container at
		once to gpu memory. */

		// Scene (camera, lights, etc)
		Matrix44 cameraView;
		Matrix44 cameraPerspective;

		// Models
		uint32 modelMatrixCount;
		Matrix44 * modelMatrixArray;
	};
	
	struct NetworkPackage
	{
		Vector3 characterPosition;
		Quaternion characterRotation;
	};

	constexpr int32 NETWORK_PACKAGE_SIZE = sizeof(NetworkPackage);
	static_assert(NETWORK_PACKAGE_SIZE <= 512, "Time to deak with bigger network packages");

	struct Network
	{
		bool32 isConnected;

		NetworkPackage inPackage;
		NetworkPackage outPackage;
	};

	struct StereoSoundSample
	{
		float left;
		float right;
	};

	struct SoundOutput
	{
		int32 sampleCount;
		StereoSoundSample * samples;
	};
}

extern "C" void
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platformInfo,
	game::Network *			network,
	game::SoundOutput *		soundOutput,
	game::RenderInfo * 		outRenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif