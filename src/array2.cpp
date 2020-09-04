template<typename T>
struct Array2
{
	s64 capacity;
	s64 count;
	T * memory;

	T & operator[](s64 index)
	{ 
		Assert(index < count);
		return memory[index];
	}

	T operator [](s64 index) const
	{
		Assert(index < count);
		return memory[index];
	}

	bool has_room_for(s64 count)
	{
		return (this->capacity - this->count - count) > 0;
	}

	T const * begin () const { return memory; } 
	T const * end () const { return memory + count; }

	T * begin () { return memory; }
	T * end () { return memory + count; }
};

template<typename T>
internal Array2<T> push_array_2(MemoryArena & allocator, s64 capacity, AllocFlags flags)
{
	Array2<T> result = {capacity, 0, push_memory<T>(allocator, capacity, flags)};
	return result;
}

template<typename T>
internal void clear_array_2(Array2<T> & array)
{
	memset(array.memory, 0, sizeof(T) * array.capacity);
	array.count = 0;
}

