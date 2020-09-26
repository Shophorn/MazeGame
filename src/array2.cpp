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

	void push(T const & value)
	{
		Assert(count < capacity);
		memory[count++] = value;
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
internal Array2<T> push_array_2(MemoryArena & allocator, s64 capacity, s32 flags)
{
	Array2<T> result = {capacity, 0, push_memory<T>(allocator, capacity, flags)};

	if (flags & ALLOC_FILL)
	{
		result.count = capacity;
	}

	return result;
}

template<typename T>
internal void clear_array_2(Array2<T> & array)
{
	memset(array.memory, 0, sizeof(T) * array.capacity);
	array.count = 0;
}

template<typename T>
internal f32 used_percent(Array2<T> const & array)
{
	f32 usedPercent = (f32)array.count / array.capacity;
	return usedPercent;
}

template<typename T>
internal s64 array_2_get_index_of(Array2<T> const & array, T const & valueInArray)
{
	s64 pointerDifference = &valueInArray - array.memory;
	Assert((pointerDifference >= 0) && (pointerDifference < array.capacity) && "Value not in array");
	return pointerDifference;
}

template<typename T>
internal void array_2_fill_from_memory(Array2<T> & array, s32 sourceCount, T const * source)
{
	Assert(sourceCount <= array.capacity);
	
	array.count = sourceCount;
	copy_memory(array.memory, source, sizeof(T) * sourceCount);
}

template<typename T>
internal void array_2_fill_uninitialized(Array2<T> & array)
{
	array.count = array.capacity;
}

template<typename T>
internal void array_2_fill_with_value(Array2<T> & array, T value)
{
	array.count = array.capacity;
	for (auto & element : array)
	{
		element = value;
	}
}