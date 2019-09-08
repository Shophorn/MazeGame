/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
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

			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count);
			MAZEGAME_ASSERT (index < count, message);
		}

		return memory [index];
	}
};

template<typename Type>
internal ArenaArray<Type>
PushArray(MemoryArena * arena, uint64 count)
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