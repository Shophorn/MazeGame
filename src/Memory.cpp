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

void copy_memory (void * destination, void const * source, u64 byteCount)
{
	memcpy(destination, source, byteCount);
};

template<typename T>
void copy_structs(T * destination, T const * source, s32 count)
{
	static_assert(std::is_trivially_destructible<T>::value);
	memcpy(destination, source, count * sizeof(T));
}

template<typename T>
void fill_array(T * memory, u64 count, T value)
{
	T * end = memory + count;
	while(memory < end)
	{
		*memory = value;
		++memory;
	}
}

internal void fill_memory(void * memory, char  value, u64 size)
{
	memset(memory, value, size);
}

template<typename T>
T convert_bytes(byte const * bytes, u64 offset)
{
	T result = *reinterpret_cast<T const *>(bytes + offset);
	return result;
}

template<typename T>
void swap(T & a, T & b)
{
	// Todo(Leo): these may  not be the requirements we require
	static_assert(std::is_trivially_constructible<T>::value);
	static_assert(std::is_trivially_destructible<T>::value);

	T temp 	= a;
	a 		= b;
	b 		= temp;
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
		AssertMsg (index < Count, "Index outside StaticArray bounds");
		return items_[index];
	}
};

// ----------------------------------------------------------------------------

enum AllocFlags : s32
{
	ALLOC_FLAGS_NONE 	= 0,
	ALLOC_FILL 			= 1,
	ALLOC_NO_CLEAR  	= 2,
	ALLOC_CLEAR 		= 4,
};

struct MemoryArena
{
	// Todo(Leo): Is this appropriate??
	static constexpr u64 defaultAlignment = sizeof(u64);

	u8 * 	memory;
	u64 	size;
	u64 	used;

	u64 	checkpoint;
};


internal void *
allocate(MemoryArena & allocator, u64 size, s32 flags = 0)
{
	/*
	Note(Leo): we align size, so start is always at aligned position.
	Todo(Leo): this assumes that the original memory is at aligned position, which it maybe is not
	*/

	u64 start 		= allocator.used;
	size 			= align_up(size, MemoryArena::defaultAlignment);
	void * result 	= allocator.memory + start;
	allocator.used 	= start + size;

	Assert(allocator.used <= allocator.size && "Out of memory");

	bool noClear = (flags & ALLOC_NO_CLEAR) != 0;
	bool yesClear = (flags & ALLOC_CLEAR) != 0;

	// // Todo(Leo): remove ALLOC_NO_CLEAR flag
	// Assert(noClear == yesClear &&  "This is clear conflict");

	if (!noClear || yesClear)
	{
		std::memset(result, 0, size);
	}

	return result;
}

template <typename T>
internal T* push_memory(MemoryArena & arena, s32 count, s32 flags)
{
	u64 size 	= sizeof(T) * count;
	T * result 	= reinterpret_cast<T*>(allocate(arena, size, flags));
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
	arena->used 		= 0;
	arena->checkpoint 	= 0;
}

internal f32 used_percent(MemoryArena & arena)
{
	f32 percent = (f32)arena.used / arena.size;
	return percent;
}

internal void push_memory_checkpoint(MemoryArena & arena)
{
	u64 used 				= arena.used;
	u64 * checkpointMemory 	= push_memory<u64>(arena, 1, 0);
	*checkpointMemory 		= arena.checkpoint;
	arena.checkpoint 		= used;
}

internal void pop_memory_checkpoint(MemoryArena & arena)
{
	Assert(arena.checkpoint > 0 && "Cannot pop memory checkpoint at 0");

	u64 * checkpointMemory 	= (u64*)(arena.memory + arena.checkpoint);
	u64 previousCheckpoint 	= *checkpointMemory;

	arena.used 				= arena.checkpoint;
	arena.checkpoint 		= previousCheckpoint;
}

// ---------------------------------------------------------------------------
// MemoryView is simplest possible dynamic memory container.

template<typename T>
struct MemoryView
{
	s32 capacity;
	s32 count;
	T * memory;
};

template <typename T>
internal MemoryView<T> push_memory_view(MemoryArena & allocator, s32 capacity)
{
	MemoryView<T> result =
	{
		capacity,
		0,
		push_memory<T>(allocator, capacity, ALLOC_NO_CLEAR)
	};
	return result;
};

template <typename T>
internal s32 memory_view_available(MemoryView<T> const & memoryView)
{
	return memoryView.capacity - memoryView.count;
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
Array<Array<T>> allocate_nested_arrays(MemoryArena & allocator, s64 outerCapacity, s64 innerCapacity, s32 innerFlags = ALLOC_FLAGS_NONE)
{
	/* Note(leo): Outer array is implicitly ALLOC_NO_CLEAR because we init all values,
	and ALLOC_FILL, because otherwise you eould just call normal allocate_array. */



	u64 innerArraySize = sizeof(T) * innerCapacity;
	u64 totalSize = outerCapacity * (innerArraySize + sizeof(Array<T>));

	void * memory = allocate(allocator, totalSize);



	Array<Array<T>> result = {reinterpret_cast<Array<T>*>(memory), (u64)outerCapacity, (u64)outerCapacity};

	T * innerArrayMemory = reinterpret_cast<T*>((byte*)memory + outerCapacity * sizeof(Array<T>));

	u64 innerCount = 0;
	if ((innerFlags & ALLOC_FILL) != 0)
	{
		innerCount = innerCapacity;
	}

	if ((innerCapacity & ALLOC_NO_CLEAR) == 0)
	{
		std::memset(innerArrayMemory, 0, sizeof(T) * innerCapacity * outerCapacity);
	}



	for(int i = 0; i < outerCapacity; ++i)
	{
		/* Note(Leo): placement new allows us to use move constuctor instead of move assignement, 
		so we can construct this array on unitialized memory. Look at Array constructors and 
		assignment operators for more info. */
		new (&result[i]) Array<T> (innerArrayMemory + i * innerCapacity, innerCount, (u64)innerCapacity);
	}


	return result;
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