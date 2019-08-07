#if !defined MAZEGAME_PLATFORM_HPP

#include "mazegame_essentials.hpp"

#include "math.cpp"
#include "vectors.cpp"
#include "MatrixBase.cpp"

struct GameInput
{
	Vector2 move;
	Vector2 look;

	bool32 zoomIn;
	bool32 zoomOut;

	real32 timeDelta;
};

struct GameMemory
{
	/* NOTICE: Both persistentMemory and transientMemory must be initlalized
	to all zeroes. */

	void * persistentMemory;
	uint64 persistentMemorySize;

	void * transientMemory;
	uint64 transientMemorySize;

	bool32 isInitialized;
};

// Note(Leo): these need to align properly
struct CameraUniformBufferObject
{
	alignas(16) Matrix44 view;
	alignas(16) Matrix44 perspective;
};


struct GameRenderInfo
{
	Matrix44 characterMatrix;

	Matrix44 cameraView;
	Matrix44 cameraPerspective;
};

struct GamePlatformInfo
{
	int32 screenWidth;
	int32 screenHeight;
};

void GameUpdateAndRender(
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platformInfo,

	GameRenderInfo * 	outRenderInfo);

#define MAZEGAME_PLATFORM_HPP
#endif