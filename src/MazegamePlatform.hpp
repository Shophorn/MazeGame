/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#include "MazegameEssentials.hpp"
#include "Array.hpp"

#include "Math.cpp"
#include "Vectors.cpp"
#include "Matrices.cpp"

#include <vector>
#include <iostream>

struct GameInput
{
	Vector2 move;
	Vector2 look;

	bool32 zoomIn;
	bool32 zoomOut;

	real32 timeDelta;
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
	std::vector<Vertex> vertices;
	std::vector<uint16> indices;
	IndexType indexType;

	// uint64 IndexSize()
	// {
	// 	switch (indexType)
	// 	{
	// 		case IndexType::UInt16: return sizeof(uint16);
	// 		case IndexType::UInt32: return sizeof(uint32);
	// 	}
	// }
};

using MeshHandle = uint64;

struct GameMemory
{
	/* NOTICE: Both persistentMemory and transientMemory must be initlalized
	to all zeroes. */

	bool32 isInitialized;

	void * persistentMemory;
	uint64 persistentMemorySize;

	void * transientMemory;
	uint64 transientMemorySize;

	void * graphicsContext;
	
	using PushMeshesFunc = void(void * graphicsContext, 
								int meshCount,
								Mesh * meshArray,
								MeshHandle * outMeshHandleArray);

	PushMeshesFunc * PushMeshesImpl;

	void
	PushMeshes(int meshCount, Mesh * meshArray, MeshHandle * outMeshHandleArray)
	{
		PushMeshesImpl(graphicsContext, meshCount, meshArray, outMeshHandleArray);
	}
};

struct GameRenderInfo
{
	/* Todo(Leo): Make a proper continer for these
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

struct GamePlatformInfo
{
	int32 screenWidth;
	int32 screenHeight;
};


struct GameNetwork
{
	bool32 isConnected;
	Vector3 characterPosition;
	Vector3 otherCharacterPosition;
};

void GameUpdateAndRender(
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platformInfo,
	GameNetwork *		network,
	GameRenderInfo * 	out_RenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif