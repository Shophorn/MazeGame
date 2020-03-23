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

///////////////////////////////////
/// 	STATIC ARRAY 			///
///////////////////////////////////

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

///////////////////////////////////////
///         MEMORY ARENA 			///
///////////////////////////////////////
/* Todo(Leo): maybe add per item deallocation for arrays eg.*/
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

///////////////////////////////////////
///         ARENA ARRAY 			///
///////////////////////////////////////

template<typename T>
struct BETTERArray
{
	BETTERArray() = default;
	
	BETTERArray(BETTERArray const &) = delete;
	void operator =(BETTERArray const &) = delete;



	BETTERArray(BETTERArray && old)
	{
		this->memory_ 	= old.memory_;
		this->capacity_ = old.capacity_;
		this->count_ 	= old.count_;

		old.memory_ 	= nullptr;
		old.capacity_ 	= 0;
		old.count_ 		= 0;
	};

	BETTERArray & operator= (BETTERArray && old)
	{
		this->memory_ 	= old.memory_;
		this->capacity_ = old.capacity_;
		this->count_ 	= old.count_;

		old.memory_ 	= nullptr;
		old.capacity_ 	= 0;
		old.count_ 		= 0;

		return *this;
	};


	BETTERArray(T * memory, u64 count, u64 capacity) :
		memory_(memory),
		count_(count),
		capacity_(capacity) {}

	T * data ()		{ return memory_; }
	T const * data ()	const { return memory_; }
	u64 count () const	{ return count_; }
	u64 capacity () const { return capacity_; }

	T & operator[] (u64 index)
	{ 
		assert(index < count_);
		return memory_[index];
	}

	T const operator[] (u64 index) const
	{ 
		assert(index < count_);
		return memory_[index];
	}

	void push(T value)
	{
		assert(count_ < capacity_);
		memory_[count_] = value;
		++count_;
	}

	void push_many(std::initializer_list<T> values)
	{
		for (auto item : values)
		{
			push(item);
		}
	}


	void flush()
	{
		count_ = 0;
	}

	T * begin() {return data();}
	T const * begin() const {return memory_;}

	T * end() {return data() + count();}
	T const * end() const {return memory_ + count();}


private:
	T * memory_;
	u64 count_;
	u64 capacity_;
};

template<typename T>
BETTERArray<T> allocate_BETTER_array(MemoryArena & arena, T const * srcBegin, T const * srcEnd)
{
	u64 capacity 	= srcEnd - srcBegin;
	u64 size 		= sizeof(T) * capacity;

	T * memory = reinterpret_cast<T*>(allocate(arena, size));

	std::copy(srcBegin, srcEnd, memory);

	return BETTERArray<T>(memory, capacity, capacity);
}

template<typename T>
BETTERArray<T> allocate_BETTER_array(MemoryArena & arena, std::initializer_list<T> values)
{
	return allocate_BETTER_array(arena, values.begin(), values.end());
}

enum AllocOperation
{
	ALLOC_EMPTY,
	ALLOC_FILL_UNINITIALIZED,
	ALLOC_CLEAR
};

template<typename T>
BETTERArray<T> allocate_BETTER_array(MemoryArena & arena, u64 capacity, AllocOperation allocOperation = ALLOC_EMPTY)
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

	return BETTERArray<T>(memory, count, capacity);
}

template<typename T>
BETTERArray<T> copy_BETTER_array(MemoryArena & arena, BETTERArray<T> const & other)
{
	u64 count = other.count();
	u64 capacity = other.capacity();
	u64 size = sizeof(T) * capacity;

	T * memory = reinterpret_cast<T*>(allocate(arena, size));

	std::copy(other.begin(), other.end(), memory);

	return BETTERArray<T>(memory, count, capacity);	
}

// template<typename TNew, typename TOld>
// BETTERArray<TNew> cast_memory_type(BETTERArray<TOld> old)
// {
// 	u64 capacity = old.capacity_ * sizeof(TOld) / sizeof(TNew);
// 	u64 count = old.count_ * sizeof(TOld) / sizeof (TNew);

// 	TNew * memory = reinterpret_cast<TNew*>(old.memory_);

// 	return BETTERArray<TNew>(memory, count, capacity);
// }

struct ArrayHeader
{
	u64 capacityInBytes;
	u64 countInBytes;
};

#define ARENA_ARRAY_TEMPLATE template<typename T, typename TIndex = u64>

ARENA_ARRAY_TEMPLATE struct ArenaArray
{
	// Todo(Leo): Do we need more constraints?
	static_assert(std::is_trivially_destructible<T>::value, "Only simple types suppported");

	/* Todo(Leo): Maybe make this also just an offset from start of arena.
	And store pointer to arena instead? */
	byte * data_;

	u64 capacity () const { return ( get_header()->capacityInBytes) / sizeof(T); }
	u64 count ()  	const { return ( get_header()->countInBytes) / sizeof(T); }

	// Todo(Leo): Maybe make these free functions too.
	T * begin() 	{ return reinterpret_cast<T*>(data_ + sizeof(ArrayHeader)); }
	T * end() 		{ return begin() + count(); }

	T const * begin() const	{ return reinterpret_cast<T const *>(data_ + sizeof(ArrayHeader)); }
	T const * end() const	{ return begin() + count(); }

	T & operator [] (TIndex index)
	{
		assert_index_validity(index);
		return begin()[index];
	}

	T const & operator [] (TIndex index) const
	{
		assert_index_validity(index);
		return begin()[index];
	}

	T & last () { return begin()[count() - 1]; }
	T const & last () const { return begin()[count() - 1]; }

private:
	inline ArrayHeader * 
	get_header () const
	{ 
		DEBUG_ASSERT(data_ != nullptr, "Invalid call to 'get_header()', ArenaArray is not initialized.");
		return reinterpret_cast<ArrayHeader*>(data_);
	}

	inline void
	assert_index_validity(TIndex index) const
	{
		#if MAZEGAME_DEVELOPMENT
			char message [200];
			sprintf(message,"Index (%d) is more than count (%d)", index, count());
			DEBUG_ASSERT (index < count(), message);
		#endif
	}
};

namespace arena_array_internal_
{
	ARENA_ARRAY_TEMPLATE internal ArrayHeader *
	get_header(ArenaArray<T, TIndex> array)
	{
		DEBUG_ASSERT(array.data_ != nullptr, "Invalid call to 'arena_array_internal_::get_header()', ArenaArray is not initialized.");
		return reinterpret_cast<ArrayHeader *>(array.data_);
	}

	ARENA_ARRAY_TEMPLATE internal void 
	set_capacity(ArenaArray<T, TIndex> array, u64 capacity)
	{
		u64 capacityInBytes = capacity * sizeof(T);
		get_header(array)->capacityInBytes = capacityInBytes;
	}

	ARENA_ARRAY_TEMPLATE internal void 
	set_count(ArenaArray<T, TIndex> array, u64 count)
	{
		u64 countInBytes = count * sizeof(T);
		get_header(array)->countInBytes = countInBytes;
	}

	ARENA_ARRAY_TEMPLATE internal void
	increment_count(ArenaArray<T, TIndex> array, u64 increment)
	{
		u64 newCount = array.count() + increment;
		set_count(array, newCount);
	}
} // arena_array_internal_

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
reserve_array(MemoryArena * arena, u64 capacity)
{
	/* Todo(Leo): make proper alignement between these two too, now it works fine,
	since header aligns on 64 anyway */
	u64 size = sizeof(ArrayHeader) + (sizeof(T) * capacity);
	byte * memory = (byte*)allocate(*arena, size);

	ArenaArray<T, TIndex> result =
	{
		.data_ = memory
	};
	arena_array_internal_::set_capacity (result, capacity);
	arena_array_internal_::set_count(result, 0);

	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, u64 capacity)
{
	auto result	= reserve_array<T, TIndex>(arena, capacity);
	arena_array_internal_::set_count(result, capacity);

	return result;
}


ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
push_array(MemoryArena * arena, const T * begin, const T * end)
{
	u64 count 	= end - begin;
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
copy_array_slice(MemoryArena * arena, ArenaArray<T, TIndex> original, TIndex start, u64 count)
{
	DEBUG_ASSERT((start + count) <= original.capacity(), "Invalid copy slice region");

	auto * begin 	= original.begin() + start;
	auto * end 		= begin + count;
	auto result 	= push_array<T, TIndex>(arena, begin, end);

	return result;
}

ARENA_ARRAY_TEMPLATE internal ArenaArray<T, TIndex>
duplicate_array(MemoryArena * arena, ArenaArray<T, TIndex> original)
{
	auto result = reserve_array<T, TIndex>(arena, original.capacity());
	arena_array_internal_::set_count(result, original.count());

	// Todo(Leo): Make own memory copies for the sake of education
	std::copy(original.begin(), original.end(), result.begin());

	return result;
}

ARENA_ARRAY_TEMPLATE internal void
reverse_arena_array(ArenaArray<T, TIndex> array)
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

ARENA_ARRAY_TEMPLATE internal TIndex
push_one(ArenaArray<T, TIndex> array, T item)
{
	DEBUG_ASSERT(array.capacity() > 0, "Cannot push, ArenaArray is not initialized!");
	DEBUG_ASSERT(array.count() < array.capacity(), "Cannot push, ArenaArray is full!");

	TIndex index = {array.count()};
	*array.end() = item;
	arena_array_internal_::increment_count(array, 1);

	return index;
}

ARENA_ARRAY_TEMPLATE internal void
push_range(ArenaArray<T, TIndex> array, const T * begin, const T * end)
{
	u64 rangeLength = end - begin;

	DEBUG_ASSERT(array.capacity() > 0, "Cannot push, ArenaArray is not initialized!");
	DEBUG_ASSERT((rangeLength + array.count()) <= array.capacity(), "Cannot push, ArenaArray is full!");

	std::copy(begin, end, array.end());
	arena_array_internal_::increment_count(array, rangeLength);
}

ARENA_ARRAY_TEMPLATE internal void
push_many(ArenaArray<T, TIndex> array, std::initializer_list<T> items)
{
	push_range(array, items.begin(), items.end());
}

ARENA_ARRAY_TEMPLATE internal void
flush_arena_array(ArenaArray<T, TIndex> array)
{
	arena_array_internal_::set_count(array, 0);
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