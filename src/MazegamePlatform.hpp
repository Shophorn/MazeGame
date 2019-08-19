/*=============================================================================
Leo Tamminen

Interface definition between Platform and Game.
===============================================================================*/

#if !defined MAZEGAME_PLATFORM_HPP

#define MAZEGAME_INCLUDE_STD_IOSTREAM 1
#include <iostream>

#include "MazegameEssentials.hpp"
#include "Array.hpp"

#include "Math.cpp"
#include "Vectors.cpp"
#include "Matrices.cpp"
#include "Quaternion.cpp"

#include <vector>



using GameButtonState = int32;
constexpr GameButtonState
	GAME_BUTTON_IS_UP = 0,
	GAME_BUTTON_WENT_DOWN = 1,
	GAME_BUTTON_WENT_UP = 2,
	GAME_BUTTON_IS_DOWN = 3;


struct GameInput
{
	Vector2 move;
	Vector2 look;

	GameButtonState jump;

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
};

/* Todo(Leo): Define INVALID_MESH_HANDLE to be zero, so it becomes default value */
using MeshHandle = uint64;

struct GameMemory
{
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

struct GamePlatformInfo
{
	int32 screenWidth;
	int32 screenHeight;
};

struct GameNetworkPackage
{
	Vector3 characterPosition;
	Quaternion characterRotation;
};

constexpr int32 GAME_NETWORK_PACKAGE_SIZE = sizeof(GameNetworkPackage);
static_assert(GAME_NETWORK_PACKAGE_SIZE <= 512, "Time to deak with bigger network packages");

struct GameNetwork
{
	bool32 isConnected;

	GameNetworkPackage inPackage;
	GameNetworkPackage outPackage;
};

struct GameStereoSoundSample
{
	float left;
	float right;
};

struct GameSoundOutput
{
	int32 sampleCount;
	GameStereoSoundSample * samples;
};


extern "C" void
GameUpdate(
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platformInfo,
	GameNetwork *		network,
	GameSoundOutput *	soundOutput,
	GameRenderInfo * 	out_RenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif