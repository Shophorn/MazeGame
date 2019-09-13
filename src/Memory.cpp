/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
struct MemoryArena
{
	byte * 	memory;
	uint64 	size;
	uint64 	used;

	byte * lastPushed = nullptr;

	uint64 Available () { return size - used; }

	class_member MemoryArena
	Create(byte * memory, uint64 size)
	{
		MemoryArena resultArena = {};
		resultArena.memory = memory;
		resultArena.size = size;
		resultArena.used = 0;
		return resultArena;
	}

	void Flush() { used = 0; }
};

template<typename Type>
struct ArenaArray
{
	Type * memory;
	uint64 count;

	Type & operator [] (int index)
	{
		{
			// TODO(Leo): Find a way to macro this away too
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


		byte * memory = arena->memory + arena->used;
		resultArray.memory = reinterpret_cast<Type *>(memory);
		resultArray.count = count;

		arena->used += requiredSize;

		/* Note(Leo): This is saved that we can assure that when we trim latest
		it actually is the latest */
		arena->lastPushed = memory;

		return resultArray;
	}
	else
	{
		MAZEGAME_ASSERT(false, "Not enough memory available in arena");
	}
}

template<typename Type> internal void
ShrinkLastArray(MemoryArena * arena, ArenaArray<Type> * array, uint64 newCount)
{
	MAZEGAME_ASSERT(arena->lastPushed == reinterpret_cast<byte *>(array->memory), "Can only shrink last pushed array!");

	uint64 delta = array->count - newCount;
	MAZEGAME_ASSERT(delta >= 0, "Can only shrink array to a smaller size");

	arena->used -= delta;
	array->count -= delta;

	// Note(Leo): We do not to change the lastPushed, because it is still the same address
}