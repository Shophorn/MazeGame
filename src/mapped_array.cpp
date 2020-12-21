template<typename T>
struct MappedArray
{
	s64 	capacity;
	s64 	count;
	s64 * 	indirectionTable;
	T * 	memory;

	T & operator[] (s64 handle)
	{
		s64 index = indirectionTable[handle];
		return memory[index];
	}

	s64 push (T const & item)
	{
		Assert(count < capacity);

		s64 index 	= count;
		count 		+= 1;

		memory[index] = item;

		s64 handle = get_free_handle();
		indirectionTable[handle] = index;

		return handle;
	}

	s64 get_free_handle()
	{
		for(s64 i = 0; i < capacity; ++i)
		{
			if (indirectionTable[i] < 0)
			{
				return i;
			}
		}
		return -1;
	}
};