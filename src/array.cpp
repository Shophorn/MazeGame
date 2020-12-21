template<typename T>
struct Array
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

	T & push(T const & value)
	{
		Assert(count < capacity);

		s64 index = count++;
		memory[index] = value;

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

/// Todo(Leo): this is maybe stupid
template<typename T>
internal Array<T> push_array(MemoryArena & allocator, s64 capacity, AllocOperation options)
{
	Array<T> result = {capacity, 0, push_memory<T>(allocator, capacity, options)};
	return result;
}

template<typename T>
internal void array_clear(Array<T> & array)
{
	memset(array.memory, 0, sizeof(T) * array.capacity);
	array.count = 0;
}

template<typename T>
internal void array_flush(Array<T> & array)
{
	array.count = 0;
}

template<typename T>
internal f32 used_percent(Array<T> const & array)
{
	f32 usedPercent = (f32)array.count / array.capacity;
	return usedPercent;
}

template<typename T>
internal s64 array_get_index_of(Array<T> const & array, T const & valueInArray)
{
	s64 pointerDifference = &valueInArray - array.memory;
	Assert((pointerDifference >= 0) && (pointerDifference < array.capacity) && "Value not in array");
	return pointerDifference;
}

template<typename T>
internal void array_fill_from_memory(Array<T> & array, s32 sourceCount, T const * source)
{
	Assert(sourceCount <= array.capacity);
	
	array.count = sourceCount;
	memory_copy(array.memory, source, sizeof(T) * sourceCount);
}

template<typename T>
internal void array_fill_uninitialized(Array<T> & array)
{
	array.count = array.capacity;
}

template<typename T>
internal void array_fill_with_value(Array<T> & array, T value)
{
	array.count = array.capacity;
	for (auto & element : array)
	{
		element = value;
	}
}

template<typename T>
internal void array_unordered_remove(Array<T> & array, s32 index)
{
	array[index] = array[array.count - 1];
	array.count -= 1;
}