#if !defined MAZEGAME_PLATFORM_HPP

#include "mazegame_essentials.hpp"

#include "math.cpp"
#include "vectors.cpp"
#include "MatrixBase.cpp"

struct GameInput
{
	real32 horizontal;
	real32 vertical;

	real32 timeDelta;
};

struct GameMemory
{
	/*
	NOTICE:

	Both persistentMemory and transientMemory must be initlalized
	to all zeroes.
	*/

	void * persistentMemory;
	int64 persistentMemorySize;

	void * transientMemory;
	int64 transientMemorySize;

	bool32 isInitialized;
};

void GameUpdateAndRender(GameInput * input, GameMemory * memory);

/*
Todo(Leo): Maybe like this, platform layer then fills these function pointers.
struct PlatfromModelHandle
{
	int handleValue;
};

PlatfromModelHandle
PlatformLoadModelFunc(const char * path);

inline decltype(PlatformLoadModelFunc) * PlatformLoadModel = nullptr;
*/

#define MAZEGAME_PLATFORM_HPP
#endif