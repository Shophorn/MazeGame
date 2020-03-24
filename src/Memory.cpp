/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:

While we are not aiming to satisfy any standard per se, when making irrelevant
but compulsory design choices, look at: 
	- https://en.cppreference.com/w/cpp/named_req/Container
=============================================================================*/
#include <initializer_list>
#include <algorithm>
#include <type_traits>
#include <cstring>

#include "Array.hpp"

// ----------------------------------------------------------------------------

template<typename T, u32 Count>
struct StaticArray
{
	static_assert(Count != 0, "Why would you do this?");

	T items_ [Count];

	constexpr u32 count () { return Count; }

	T * begin() { return items_; }
	T * end() 	{ return items_ + Count; }

	T & operator [] (u32 index)
	{
		DEBUG_ASSERT (index < Count, "Index outside StaticArray bounds");
		return items_[index];
	}
};

// ----------------------------------------------------------------------------

struct MemoryArena
{
	// Todo(Leo): Is this appropriate??
	static constexpr u64 defaultAlignment = sizeof(u64);

	void * 	memory;
	u64 	size;
	u64 	used;

	void * next () { return (u8*)memory + used; }
	u64 available () { return size - used; }
};

internal void *
allocate(MemoryArena & arena, s64 size, bool clear = false)
{
	size = align_up_to(size, MemoryArena::defaultAlignment);

	DEBUG_ASSERT(size <= arena.available(), "Not enough memory available in MemoryArena");

	void * result = arena.next();
	arena.used += size;

	if (clear)
	{
		std::memset(result, 0, size);
	}

	return result;

}

internal MemoryArena
make_memory_arena(byte * memory, u64 size)
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

// ----------------------------------------------------------------------------

template<typename T>
Array<T> allocate_array(MemoryArena & arena, T const * srcBegin, T const * srcEnd)
{
	u64 capacity 	= srcEnd - srcBegin;
	u64 size 		= sizeof(T) * capacity;

	T * memory = reinterpret_cast<T*>(allocate(arena, size));

	std::copy(srcBegin, srcEnd, memory);

	return Array<T>(memory, capacity, capacity);
}

template<typename T>
Array<T> allocate_array(MemoryArena & arena, std::initializer_list<T> values)
{
	return allocate_array(arena, values.begin(), values.end());
}


// Todo(Leo): Extra stupid to use
enum AllocOperation
{
	ALLOC_EMPTY,
	ALLOC_FILL_UNINITIALIZED,
	ALLOC_CLEAR
};

template<typename T>
Array<T> allocate_array(MemoryArena & arena, u64 capacity, AllocOperation allocOperation = ALLOC_EMPTY)
{
	u64 size = sizeof(T) * capacity;

	T * memory = reinterpret_cast<T*>(allocate(arena, size));

	u64 count = 0;
	switch (allocOperation)
	{
		case ALLOC_EMPTY:
			count = 0;
			break;

		case ALLOC_FILL_UNINITIALIZED:
			count = capacity;
			break;

		case ALLOC_CLEAR:
			count = capacity;
			std::memset(memory, 0, size);
			break;

	}

	return Array<T>(memory, count, capacity);
}

template<typename T>
Array<T> copy_array(MemoryArena & arena, Array<T> const & other)
{	
	if(other.data() == nullptr)
	{
		return {};
	}

	u64 count = other.count();
	u64 capacity = other.capacity();
	u64 size = sizeof(T) * capacity;

	T * memory = reinterpret_cast<T*>(allocate(arena, size));

	std::copy(other.begin(), other.end(), memory);

	return Array<T>(memory, count, capacity);	
}

template<typename T>
void reverse_BETTER_array(Array<T> & array)
{
	T temp = array[0];
	u64 halfCount = array.count() / 2;
	u64 lastIndex = array.count() - 1;

	for (int i = 0; i < halfCount; ++i)
	{
		temp = array[i];
		array[i] = array[lastIndex -i];
		array[lastIndex - i] = temp;
	}	
}