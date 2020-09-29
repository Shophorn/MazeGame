/*
Leo Tamminen

Memory managing things in Friendsimulator
*/

// Note(Leo): We kinda have expected to always use 64 bit systems.
static_assert(sizeof(void*) == sizeof(u64));

/// ----------------- RANDOM UTITLITES --------------------------------

internal u64
memory_align_up(u64 size, u64 alignment)
{
    /*
    Divide to get number of full multiples of alignment
    Add one so we end up bigger
    Multiply to get actual size, that is both bigger AND a multiple of alignment
    */

    size /= alignment;
    size += 1;
    size *= alignment;
    return size;
}

void memory_copy (void * destination, void const * source, u64 byteCount)
{
	memcpy(destination, source, byteCount);
};

internal void memory_set(void * memory, char  value, u64 size)
{
	memset(memory, value, size);
}

template<typename T>
T memory_convert_bytes_to(byte const * bytes, u64 offset)
{
	T result = *reinterpret_cast<T const *>(bytes + offset);
	return result;
}

template<typename T>
void memory_swap(T & a, T & b)
{
	// Todo(Leo): these may  not be the requirements we require
	static_assert(std::is_trivially_constructible<T>::value);
	static_assert(std::is_trivially_destructible<T>::value);

	T temp 	= a;
	a 		= b;
	b 		= temp;
}

// ------------- MEMORY ARENA ----------------------------------------

struct MemoryArena
{
	// Todo(Leo): Is this appropriate??
	static constexpr u64 defaultAlignment = sizeof(u64);

	u8 * 	memory;
	u64 	size;
	u64 	used;

	u64 	checkpoint;
};

internal MemoryArena
memory_arena(byte * memory, u64 size)
{
	MemoryArena resultArena =
	{
		.memory = memory,
		.size = size,
		.used = 0,
	};
	return resultArena;
}

internal void *
memory_arena_allocate(MemoryArena & allocator, u64 size)
{
	Assert((allocator.used + size) <= allocator.size);

	/*
	Note(Leo): we align size, so start is always at aligned position.
	Todo(Leo): this assumes that the original memory is at aligned position, which it maybe is not
	*/

	u64 start 		= allocator.used;
	size 			= memory_align_up(size, MemoryArena::defaultAlignment);
	void * result 	= allocator.memory + start;
	allocator.used 	= start + size;

	return result;
}



internal void
flush_memory_arena(MemoryArena * arena)
{
	arena->used 		= 0;
	arena->checkpoint 	= 0;
}

internal f32 used_percent(MemoryArena & arena)
{
	f32 percent = (f32)arena.used / arena.size;
	return percent;
}

/// ------------- MEMORY CHECKPOINT FUNCTIONS ---------------------------------------

struct MemoryCheckpoint
{
	u64 used;
	bool popped;

	~MemoryCheckpoint()
	{
		Assert(popped && "MemoryCheckpoint was not returned to arena");
	}
};

internal MemoryCheckpoint memory_push_checkpoint(MemoryArena & arena)
{
	MemoryCheckpoint checkpoint = {arena.used};
	return checkpoint;
}

internal void memory_pop_checkpoint(MemoryArena & arena, MemoryCheckpoint & checkpoint)
{
	arena.used 			= checkpoint.used;
	checkpoint.popped 	= true;
}


// internal void push_memory_checkpoint(MemoryArena & arena)
// {
// 	u64 used 				= arena.used;
// 	u64 * checkpointMemory 	= push_memory<u64>(arena, 1, ALLOC_GARBAGE);
// 	*checkpointMemory 		= arena.checkpoint;
// 	arena.checkpoint 		= used;
// }

// internal void pop_memory_checkpoint(MemoryArena & arena)
// {
// 	Assert(arena.checkpoint > 0 && "Cannot pop memory checkpoint at 0");

// 	u64 * checkpointMemory 	= (u64*)(arena.memory + arena.checkpoint);
// 	u64 previousCheckpoint 	= *checkpointMemory;

// 	arena.used 				= arena.checkpoint;
// 	arena.checkpoint 		= previousCheckpoint;
// }

/// ------------- PUSH MEMORY FUNCTIONS ---------------------------------------

enum AllocOperation : s32
{
	ALLOC_GARBAGE,
	ALLOC_ZERO_MEMORY,
// 	ALLOC_DEFAULT_INITIALIZE,
};

template <typename T>
internal T* push_memory(MemoryArena & arena, s32 count, AllocOperation options)
{
	u64 size 	= sizeof(T) * count;
	T * result 	= reinterpret_cast<T*>(memory_arena_allocate(arena, size));

	if (options == ALLOC_ZERO_MEMORY)
	{
		memset(result, 0, size);
	}

	// Note(Leo): This is here, but we do not have use case for it.
	// if (options == ALLOC_DEFAULT_INITIALIZE)
	// {
	// 	for (s32 i = 0; i < count; ++i)
	// 	{
	// 		result[i] = {};
	// 	}
	// }

	return result;	
}

template <typename T>
internal T * push_and_copy_memory(MemoryArena & arena, s32 count, T const * source)
{
	T * result = push_memory<T>(arena, count, ALLOC_GARBAGE);
	memory_copy(result, source, count * sizeof(T));
	return result;
};

template<typename TFirst, typename ... TOthers>
internal void push_multiple_memories(MemoryArena & arena, s32 count, AllocOperation options, TFirst first, TOthers... others)
{
	using type = pointer_strip<TFirst>;
	static_assert(is_same_type<type**, TFirst>, "first must be a pointer to pointer type");

	*first = push_memory<type>(arena, count, options);

	if constexpr (sizeof...(TOthers) > 0)
	{
		push_multiple_memories(arena, count, options, others...);
	}
}

// ----------------------------------------------------------------------------

