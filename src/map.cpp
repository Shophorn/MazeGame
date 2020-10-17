template<typename T>
struct Map
{
	s32 capacity;
	s32 count;	

	s32 * 	mappedIndices;
	T * 	memory;

	T & operator[](s32 index)
	{
		s32 mappedIndex = mappedIndices[index];
		return memory[index];
	}
};

template<typename T>
internal s32 map_push(Map<T> & map, T value)
{
	s32 mappedIndex = map.count;
	map.count 		+= 1;

	map.memory[mappedIndex] = value;

	s32 handleIndex = 0;

	while(map.mappedIndices[handleIndex] >= 0)
	{
		handleIndex += 1;
	}

	map.mappedIndices[handleIndex] = mappedIndex;
	return handleIndex;
}

template<typename T>
internal void map_remove(Map<T> & map, s32 handle)
{
	s32 mappedIndex 		= map.mappedIndices[handle];
	map.memory[mappedIndex] = map.memory[map.count - 1];
	map.count 				-= 1;
	map.mappedIndices 		= -1;
}