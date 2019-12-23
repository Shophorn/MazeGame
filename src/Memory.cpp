/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
#include <cstring>
#include <initializer_list>

using default_index_type = uint64;

template<typename T, typename TIndex = default_index_type>
struct ArenaArray
{
	/* Todo(Leo): Copying this and forgetting it and using the copy will
	be bad since count and capacity are not synchronized then.
	
	A) store count and capacity in memory arena just before data.
	B) make this uncopyable and use move semantics.

	A seems to me as more straightforward, from the users viewpoint,
	at least for now. It allows easy use of copies of this array.

	Also if we make the pointer to point to arena, and store offset from
	it's beginning, we could then easily verify from here whether the 
	arena is still in use. We should not need to do that ever, if we are
	careful (we should be anyway) with arena deallocation and game state
	changes.
	*/

	/* Todo(Leo): This class supports only classes that are simple such as
	pod types and others that do not need destructors or other unitilization
	only.
	
	Should we do somthing about that, at least to enforce T to be simple?
	*/

	/* Todo(Leo): Also hide implementation. C-style, now that I have found
	myself liking c-style function calls :)
	*/

	/* Todo(Leo): Maybe make this also just an offset from start of arena.
	And store pointer to arena instead? */
	T * data;
	uint64 count;
	uint64 capacity;

	T & operator [] (TIndex index)
	{
		assert_index_validity(index);
		return data [index];
	}

	const T & operator [] (TIndex index) const
	{
		assert_index_validity(index);
		return data [index];
	}

private:
	inline void
	assert_index_validity(TIndex index) const
	{
		#if MAZEGAME_DEVELOPMENT
			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count);
			MAZEGAME_ASSERT (index < count, message);
		#endif
	}
};

template<typename T, typename TIndex>
internal TIndex
push_one(ArenaArray<T, TIndex> * array, T item)
{
	MAZEGAME_ASSERT(array->count < array->capacity, "Cannot push, ArenaArray is full!");

	TIndex index = {array->count++};
	array->data[index] = item;

	return index;
}

template<typename T, typename TIndex>
internal void
flush_arena_array(ArenaArray<T, TIndex> * array)
{
	array->count = 0;
}


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

	byte * next () { return memory + used; }
	uint64 available () { return size - used; }
};

internal MemoryArena
make_memory_arena(byte * memory, uint64 size)
{
	MemoryArena resultArena = {};

	resultArena.memory = memory;
	resultArena.size = size;
	resultArena.used = 0;

	return resultArena;
}

internal byte *
reserve_from_memory_arena(MemoryArena * arena, uint64 requestedSize)
{
	// Todo[Memory](Leo): Alignment
	MAZEGAME_ASSERT(requestedSize <= arena->available(), "Not enough memory available in MemoryArena");

	byte * result = arena->next();
	arena->used += requestedSize;
	
	return result; 	
}

internal void
flush_memory_arena(MemoryArena * arena)
{
	arena->used = 0;
}

internal void
clear_memory_arena(MemoryArena * arena)
{
	arena->used = 0;
	memset(arena->memory, 0, arena->size);
}

/* Todo(Leo): 'fillWithUninitialized' was given true by default when capacity was
added so that previous usages wouldn't break. It does not need to be that way, so
rethink it someday. */
template<typename Type, typename IndexType = default_index_type>
internal ArenaArray<Type, IndexType>
push_array(MemoryArena * arena, uint64 capacity, bool32 fillWithUninitialized = true)
{
	// byte * memory = arena->Reserve(sizeof(Type) * capacity);
	byte * memory = reserve_from_memory_arena(arena, sizeof(Type) * capacity);

	ArenaArray<Type, IndexType> resultArray =
	{
		.data 		= reinterpret_cast<Type *>(memory),
		.count 		= fillWithUninitialized ? capacity : 0,
		.capacity 	= capacity
	};
	return resultArray;
}


template<typename T>
internal ArenaArray<T>
push_array(MemoryArena * arena, std::initializer_list<T> items)
{
	uint64 count = items.size();
	byte * memory = reserve_from_memory_arena(arena, sizeof(T) * count);

	ArenaArray<T> result = 
	{
		.data 		= reinterpret_cast<T*>(memory),
		.count 		= count,
		.capacity 	= count,
	};

	// Todo(Leo): use memcpy
	for (int i = 0; i < count; ++i)
	{
		result[i] = *(items.begin() + i);
	}

	return result;
}

template<typename T, typename TIndex>
internal ArenaArray<T, TIndex>
duplicate_arena_array(MemoryArena * arena, ArenaArray<T, TIndex> * original)
{
	uint64 capacity = original->capacity;
	uint64 count = original->count;

	byte * memory = reserve_from_memory_arena(arena, sizeof(T) * capacity);

	ArenaArray<T> result = 
	{
		.data 		= reinterpret_cast<T*>(memory),
		.count 		= count,
		.capacity 	= capacity,
	};

	// Todo(Leo): use memcpy
	for (int i = 0; i < count; ++i)
	{
		result[i] = (*original)[i];
	}

	return result;
}

template <typename T, typename TIndex>
internal void
reverse_arena_array(ArenaArray<T, TIndex> * array)
{
	T temp = (*array)[0];
	uint64 halfCount = array->count / 2;
	uint64 lastIndex = array->count - 1;

	for (int i = 0; i < halfCount; ++i)
	{
		temp = (*array)[i];
		(*array)[i] = (*array)[lastIndex -i];
		(*array)[lastIndex - i] = temp;
	}
}

#if MAZEGAME_DEVELOPMENT
internal float 
get_arena_used_percent(MemoryArena * memoryArena)
{
	real64 used = memoryArena->used;
	real64 total = memoryArena->size;
	real32 result = static_cast<real32>(used / total * 100.0);
	return result;
}
#endif