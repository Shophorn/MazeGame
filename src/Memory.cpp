/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:
=============================================================================*/
#include <cstring>
#include <initializer_list>
#include <algorithm>
#include <type_traits>

///////////////////////////////////
/// 	STATIC ARRAY 			///
///////////////////////////////////

template<typename T, int32 Count>
struct StaticArray
{
	T _items [Count];

	constexpr int32 count () { return Count; }

	T & operator [] (int32 index)
	{
		DEVELOPMENT_ASSERT (index < Count, "Index outside StaticArray bounds");
		return _items[index];
	}
};

template<typename T, int32 Count>
T * begin(StaticArray<T, Count> array) { return array._items; }

template<typename T, int32 Count>
T * end(StaticArray<T, Count> array) { return array._items + Count; }


///////////////////////////////////////
///         MEMORY ARENA 			///
///////////////////////////////////////
/* Todo(Leo): maybe add per item deallocation for arrays eg.*/
struct MemoryArena
{
	// Todo(Leo): Is this appropriate??
	static constexpr uint64 defaultAlignment = sizeof(uint64);

	byte * 	memory;
	uint64 	size;
	uint64 	used;

	byte * next () { return memory + used; }
	uint64 available () { return size - used; }
};

internal byte *
reserve_from_memory_arena(MemoryArena * arena, uint64 size)
{
	size = align_up_to(size, MemoryArena::defaultAlignment);
	
	DEVELOPMENT_ASSERT(size <= arena->available(), "Not enough memory available in MemoryArena");

	byte * result = arena->next();
	arena->used += size;

	return result;
}

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

internal void
flush_memory_arena(MemoryArena * arena)
{
	arena->used = 0;
}

internal void
clear_memory_arena(MemoryArena * arena)
{
	arena->used = 0;
	std::fill_n(arena->memory, arena->size, 0);
}

template<typename T> internal T *
push_empty_struct(MemoryArena * arena)
{
	T * result = reinterpret_cast<T*>(reserve_from_memory_arena(arena, sizeof(T)));
	return result;
}


///////////////////////////////////////
///         ARENA ARRAY 			///
///////////////////////////////////////

using default_index_type = uint64;

struct ArrayHeader
{
	uint64 capacityInBytes;
	uint64 countInBytes;
};

#define ARENA_ARRAY_TEMPLATE template<typename T, typename TIndex = default_index_type>

ARENA_ARRAY_TEMPLATE struct ArenaArray
{
	/* Todo(Leo): This class supports only classes that are simple such as
	pod types and others that do not need destructors or other unitilization
	only.
	
	Should we do somthing about that, at least to enforce T to be simple?
	*/

	/* Todo(Leo): Also hide implementation. C-style maybe, now that I have found
	myself liking c-style function calls :)
	*/

	/* Todo(Leo): Maybe make this also just an offset from start of arena.
	And store pointer to arena instead? */
	byte * _data = nullptr;

	uint64 capacity () 		{ return (get_header()->capacityInBytes) / sizeof(T); }
	uint64 count ()  		{ return (get_header()->countInBytes) / sizeof(T); }

	// Todo(Leo): Maybe make these free functions too.
	T * begin() 			{ return reinterpret_cast<T*>(_data + sizeof(ArrayHeader)); }
	T * end() 				{ return begin() + count(); }

	T & operator [] (TIndex index)
	{
		assert_index_validity(index);
		return begin()[index];
	}

private:
	inline ArrayHeader * 
	get_header ()
	{ 
		DEVELOPMENT_ASSERT(_data != nullptr, "Invalid call to 'get_header()', ArenaArray is not initialized.");
		return reinterpret_cast<ArrayHeader*>(_data);
	}

	inline void
	assert_index_validity(TIndex index)
	{
		#if MAZEGAME_DEVELOPMENT
			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count());
			DEVELOPMENT_ASSERT (index < count(), message);
		#endif
	}
};

namespace array_internal
{
	ARENA_ARRAY_TEMPLATE internal ArrayHeader *
	get_header(ArenaArray<T, TIndex> array)
	{
		DEVELOPMENT_ASSERT(array._data != nullptr, "Invalid call to 'array_internal::get_header()', ArenaArray is not initialized.");
		return reinterpret_cast<ArrayHeader *>(array._data);
	}

	ARENA_ARRAY_TEMPLATE internal void 
	set_capacity(ArenaArray<T, TIndex> array, uint64 capacity)
	{
		uint64 capacityInBytes = capacity * sizeof(T);
		get_header(array)->capacityInBytes = capacityInBytes;
	}

	ARENA_ARRAY_TEMPLATE internal void 
	set_count(ArenaArray<T, TIndex> array, uint64 count)
	{
		uint64 countInBytes = count * sizeof(T);
		get_header(array)->countInBytes = countInBytes;
	}

	ARENA_ARRAY_TEMPLATE internal void
	increment_count(ArenaArray<T, TIndex> array, uint64 increment)
	{
		uint64 newCount = array.count() + increment;
		set_count(array, newCount);
	}
} // array_internal

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
reserve_array(MemoryArena * arena, uint64 capacity)
{
	/* Todo(Leo): make proper alignement between these two too, now it works fine,
	since header aligns on 64 anyway */
	uint64 size = sizeof(ArrayHeader) + (sizeof(T) * capacity);
	byte * memory = reserve_from_memory_arena(arena, size);

	ArenaArray<T, TIndex> result =
	{
		._data = memory
	};
	array_internal::set_capacity (result, capacity);
	array_internal::set_count(result, 0);

	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, uint64 capacity)
{
	auto result	= reserve_array<T, TIndex>(arena, capacity);
	array_internal::set_count(result, capacity);

	return result;
}


ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, const T * begin, const T * end)
{
	uint64 count 	= end - begin;
	auto result 	= push_array<T, TIndex>(arena, count);
	std::copy(begin, end, result.begin());

	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, std::initializer_list<T> items)
{
	auto result = push_array<T, TIndex>(arena, items.begin(), items.end());
	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
copy_array_slice(MemoryArena * arena, ArenaArray<T, TIndex> original, TIndex start, uint64 count)
{
	DEVELOPMENT_ASSERT((start + count) <= original.capacity(), "Invalid copy slice region");

	auto * begin 	= original.begin() + start;
	auto * end 		= begin + count;
	auto result 	= push_array<T, TIndex>(arena, begin, end);

	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
duplicate_array(MemoryArena * arena, ArenaArray<T, TIndex> original)
{
	auto result = reserve_array<T, TIndex>(arena, original.capacity());
	array_internal::set_count(result, original.count());

	// Todo(Leo): Make own memory copies for the sake of education
	std::copy(original.begin(), original.end(), result.begin());

	return result;
}

ARENA_ARRAY_TEMPLATE internal void
reverse_arena_array(ArenaArray<T, TIndex> array)
{
	T temp = array[0];
	uint64 halfCount = array.count() / 2;
	uint64 lastIndex = array.count() - 1;

	for (int i = 0; i < halfCount; ++i)
	{
		temp = array[i];
		array[i] = array[lastIndex -i];
		array[lastIndex - i] = temp;
	}
}

ARENA_ARRAY_TEMPLATE internal TIndex
push_one(ArenaArray<T, TIndex> array, T item)
{
	DEVELOPMENT_ASSERT(array.capacity() > 0, "Cannot push, ArenaArray is not initialized!");
	DEVELOPMENT_ASSERT(array.count() < array.capacity(), "Cannot push, ArenaArray is full!");

	TIndex index = {array.count()};
	*array.end() = item;
	array_internal::increment_count(array, 1);

	return index;
}

ARENA_ARRAY_TEMPLATE internal void
push_range(ArenaArray<T, TIndex> array, const T * begin, const T * end)
{
	uint64 rangeLength = end - begin;

	DEVELOPMENT_ASSERT(array.capacity() > 0, "Cannot push, ArenaArray is not initialized!");
	DEVELOPMENT_ASSERT((rangeLength + array.count()) <= array.capacity(), "Cannot push, ArenaArray is full!");

	std::copy(begin, end, array.end());
	array_internal::increment_count(array, rangeLength);
}

ARENA_ARRAY_TEMPLATE internal void
push_many(ArenaArray<T, TIndex> array, std::initializer_list<T> items)
{
	push_range(array, items.begin(), items.end());
}

ARENA_ARRAY_TEMPLATE internal void
flush_arena_array(ArenaArray<T, TIndex> array)
{
	array_internal::set_count(array, 0);
}

#undef ARENA_ARRAY_TEMPLATE


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