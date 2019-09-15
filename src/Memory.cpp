/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
using default_index_type = uint64;

template<typename Type, typename IndexType = default_index_type>
struct ArenaArray
{
	Type * data;
	uint64 count;

	Type & operator [] (IndexType index)
	{
		#if MAZEGAME_DEVELOPMENT
			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count);
			MAZEGAME_ASSERT (index < count, message);
		#endif

		return data [index];
	}
};

/* 
Todo(Leo): Think of hiding some members of this, eg. 'lastPushed'.

Todo(Leo): Think if we need to flush more smartly, or actually keep track of
items more smartly maybe like track item generations, and maybe add per item
deallocation
*/
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

template<typename Type, typename IndexType = default_index_type>
internal ArenaArray<Type, IndexType>
PushArray(MemoryArena * arena, uint64 count)
{
	uint64 requiredSize = sizeof(Type) * count;

	MAZEGAME_ASSERT(requiredSize <= arena->Available(), "Not enough memory available in arena");

	ArenaArray<Type, IndexType> resultArray = {};

	// Todo[Memory](Leo): Alignment
	byte * memory = arena->memory + arena->used;
	resultArray.data = reinterpret_cast<Type *>(memory);
	resultArray.count = count;

	arena->used += requiredSize;

	/* Note(Leo): This is saved that we can assure that when we trim latest
	it actually is the latest */
	arena->lastPushed = memory;

	return resultArray;
}

template<typename Type> internal void
ShrinkLastArray(MemoryArena * arena, ArenaArray<Type> * array, uint64 newCount)
{
	MAZEGAME_ASSERT(arena->lastPushed == reinterpret_cast<byte *>(array->data), "Can only shrink last pushed array!");

	uint64 delta = array->count - newCount;
	MAZEGAME_ASSERT(delta >= 0, "Can only shrink array to a smaller size");

	arena->used -= delta;
	array->count -= delta;

	// Note(Leo): We do not to change the lastPushed, because it is still the same address
}

#if MAZEGAME_DEVELOPMENT
internal real32 
GetArenaUsedPercent(MemoryArena * memoryArena)
{
	real64 used = memoryArena->used;
	real64 total = memoryArena->size;
	real32 result = static_cast<real32>(used / total * 100.0);
	return result;
}
#endif