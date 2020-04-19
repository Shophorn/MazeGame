/*=============================================================================
Leo Tamminen

Memory managing things in :MAZEGAME:

While we are not aiming to satisfy any standard per se, when making irrelevant
but compulsory design choices, look at: 
	- https://en.cppreference.com/w/cpp/named_req/Container
=============================================================================*/
#include <initializer_list>
#include <type_traits>
#include <cstring>

#include "Array.hpp"

// Note(Leo): We kinda hav expected to always use 64 bit systems.
static_assert(sizeof(void*) == sizeof(u64));

internal u64
align_up(u64 size, u64 alignment)
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

void copy_memory (void * dst, void const * src, u64 count)
{
	memcpy(dst, src, count);
};

template<typename T>
T convert_bytes(byte const * bytes, u64 offset)
{
	T result = *reinterpret_cast<T const *>(bytes + offset);
	return result;
}

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
	size = align_up(size, MemoryArena::defaultAlignment);

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

internal f32 used_percent(MemoryArena & arena)
{
	f32 percent = (f32)arena.used / arena.size;
	return percent;
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


enum AllocFlags : s32
{
	ALLOC_FLAGS_NONE 	= 0,
	ALLOC_FILL 			= 1,
	ALLOC_NO_CLEAR  	= 2,
};

template <typename T>
Array<T> allocate_array(MemoryArena & allocator, u64 capacity, s32 flags = ALLOC_FLAGS_NONE)
{
	u64 size = sizeof(T) * capacity;
	T * memory = reinterpret_cast<T*>(allocate(allocator, size));

	u64 count = 0;
	if ((flags & ALLOC_FILL) != 0)
	{
		count = capacity;
	}

	// Notice: Logic is inverted here
	if ((flags & ALLOC_NO_CLEAR) == 0)
	{
		std::memset(memory, 0, size);
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
void reverse_array(Array<T> & array)
{
	// Note(Leo): nothing to reverse
	if (array.count() < 2)
		return;

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