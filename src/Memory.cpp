/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
#include <cstring>

using default_index_type = uint64;

template<typename Type, typename IndexType = default_index_type>
struct ArenaArray
{
	Type * data;
	uint64 count;
	uint64 capacity;

	// Type * begin() { return data; }
	// Type * end() { return data + count; }	

	Type & operator [] (IndexType index)
	{
		#if MAZEGAME_DEVELOPMENT
			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count);
			MAZEGAME_ASSERT (index < count, message);
		#endif

		return data [index];
	}

	IndexType Push(Type item)
	{
		std::cout << "[ArenaArray]: count = " << count << ", capacity = " << capacity << "\n";

		MAZEGAME_ASSERT(count < capacity, "Cannot push, ArenaArray is full!");

		IndexType index = {count++};
		data[index] = item;

		return index;
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

	byte * next () { return memory + used; }
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

	byte * Reserve(int requestedSize)
	{
		// Todo[Memory](Leo): Alignment
		MAZEGAME_ASSERT(requestedSize <= Available(), "Not enough memory available in MemoryArena");

		byte * result = next();
		used += requestedSize;
		
		/* Note(Leo): This is saved that we can assure that when we trim latest
		it actually is the latest */
		lastPushed = result;

		return result; 
	}

	void Flush()
	{ 
		used = 0;
		lastPushed = nullptr; 
	}
	
	void Clear()
	{
		used  = 0;
		lastPushed = nullptr;
		memset (memory, 0, size);
	}
};


/* Todo(Leo): 'fillWithUninitialized' was given true by default when capacity was
added so that previous usages wouldn't break. It does not need to be that way, so
rethink it someday. */
template<typename Type, typename IndexType = default_index_type>
internal ArenaArray<Type, IndexType>
PushArray(MemoryArena * arena, uint64 capacity, bool32 fillWithUninitialized = true)
{
	byte * memory = arena->Reserve(sizeof(Type) * capacity);

	ArenaArray<Type, IndexType> resultArray =
	{
		.data 		= reinterpret_cast<Type *>(memory),
		.count 		= fillWithUninitialized ? capacity : 0,
		.capacity 	= capacity
	};
	return resultArray;
}




template<typename Type> internal void
ShrinkLastArray(MemoryArena * arena, ArenaArray<Type> * array, uint64 newSize)
{
	MAZEGAME_ASSERT(arena->lastPushed == reinterpret_cast<byte *>(array->data), "Can only shrink last pushed array!");

	uint64 delta = array->count - newSize;
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