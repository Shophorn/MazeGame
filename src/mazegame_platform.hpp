#if !defined MAZEGAME_PLATFORM_HPP

#include "mazegame_essentials.hpp"
#include "Array.hpp"

#include "math.cpp"
#include "vectors.cpp"
#include "MatrixBase.cpp"

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
	uint32 indexSize;
};


using MeshHandle = uint64;
using PushMeshFunc = MeshHandle(void * platformContext, Mesh * mesh);

struct GameMemory
{
	/* NOTICE: Both persistentMemory and transientMemory must be initlalized
	to all zeroes. */

	bool32 isInitialized;

	void * persistentMemory;
	uint64 persistentMemorySize;

	void * transientMemory;
	uint64 transientMemorySize;

	void * platformContext;
	PushMeshFunc * PushMeshImpl;

	MeshHandle PushMesh(Mesh * mesh)
	{
		MeshHandle result = PushMeshImpl(platformContext, mesh);
		return result;
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


void GameUpdateAndRender(
	GameInput * 		in_Input,
	GameMemory * 		in_Memory,
	GamePlatformInfo * 	in_PlatformInfo,
	GameRenderInfo * 	out_RenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif