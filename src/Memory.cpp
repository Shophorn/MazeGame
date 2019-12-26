/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
#include <cstring>
#include <initializer_list>
#include <algorithm>
#include <type_traits>

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
	T * 	data;
	uint64 	capacity;
	uint64 	count;

	T * begin() { return data; }
	T * end() { return data + count; }

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

/* Note(Leo): Like a constructor, use this to make one of this so whenever
we decide to change layout of ArenaArray we only change this function and
get a nice compiler error.

Todo(Leo): Really, should we do a proper constructor? */
template<typename T, typename TIndex = default_index_type>
internal ArenaArray<T, TIndex>
make_arena_array(T * data, uint64 capacity, uint64 count = MaxValue<uint64>)
{
	ArenaArray<T, TIndex> result =
	{
		.data 		= data,
		.capacity 	= capacity,
		.count 		= Min(capacity, count),
	};
	return result;
}

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
	MemoryArena resultArena =
	{
		.memory = memory,
		.size = size,
		.used = 0,
	};
	return resultArena;
}

internal void *
reserve_from_memory_arena(MemoryArena * arena, uint64 size)
{
	// Todo[Memory](Leo): Alignment
	MAZEGAME_ASSERT(size <= arena->available(), "Not enough memory available in MemoryArena");

	void * result = reinterpret_cast<void*>(arena->next());
	arena->used += size;
	
	return result; 		
}

template <typename T>
internal T *
reserve_from_memory_arena(MemoryArena * arena, uint64 count)
{
	uint64 size = sizeof(T) * count;

	// Todo[Memory](Leo): Alignment
	MAZEGAME_ASSERT(size <= arena->available(), "Not enough memory available in MemoryArena");

	T * result = reinterpret_cast<T*>(arena->next());
	arena->used += size;
	
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
template<typename T, typename TIndex = default_index_type>
internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, uint64 capacity, bool32 fillWithUninitialized = true)
{
	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	uint64 count 	= fillWithUninitialized ? capacity : 0;
	auto result 	= make_arena_array<T, TIndex>(memory, capacity, count);
	
	return result;
}

template<typename T, typename TIndex = default_index_type>
internal ArenaArray<T, TIndex>
reserve_array(MemoryArena * arena, uint64 capacity)
{
	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	uint64 count 	= 0;
	auto result 	= make_arena_array<T, TIndex>(memory, capacity, count);

	return result;
}

template<typename T>
internal ArenaArray<T>
push_array(MemoryArena * arena, std::initializer_list<T> items)
{
	uint64 capacity = items.size();
	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	auto result 	= make_arena_array(memory, capacity);

	std::copy(items.begin(), items.end(), result.begin());

	return result;
}

template<typename T>
internal ArenaArray<T>
push_array(MemoryArena * arena, const T * begin, const T * end)
{
	uint64 capacity = end - begin;
	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	auto result 	= make_arena_array(memory, capacity);

	std::copy(begin, end, result.begin());

	return result;
}

template<typename T, typename TIndex>
internal ArenaArray<T, TIndex>
copy_array_slice(MemoryArena * arena, ArenaArray<T, TIndex> * original, TIndex start, uint64 count)
{
	MAZEGAME_ASSERT((start + count) <= original->capacity, "Invalid copy slice region");

	uint64 capacity = count;
	std::cout << "copying array slice, capacity = " << capacity << "\n"; 

	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	auto result 	= make_arena_array(memory, capacity);

	auto * begin 	= original->begin() + start;
	auto * end 		= begin + count;

	std::copy(begin, end, result.begin());

	return result;
}

template<typename T, typename TIndex>
internal ArenaArray<T, TIndex>
duplicate_array(MemoryArena * arena, ArenaArray<T, TIndex> * original)
{
	uint64 capacity = original->capacity;
	uint64 count 	= original->count;

	T * memory 		= reserve_from_memory_arena<T>(arena, capacity);
	auto result 	= make_arena_array(memory, capacity, count);

	std::copy(original->begin(), original->end(), result.begin());

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

template<typename TNew, typename TOld>
internal ArenaArray<TNew>
cast_array_type(ArenaArray<TOld> * original)
{
	std::cout << "cast_array_type()\n";

	auto result = make_arena_array<TNew>(	reinterpret_cast<TNew*>(original->data), 
											original->capacity,
											original->count);
	return result;
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