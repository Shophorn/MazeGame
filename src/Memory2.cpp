// struct MemoryArena
// {
// 	static void * allocate(MemoryArena * allocator);
// 	static void flush();

// private:	
// };


// template <typename T>
// struct Array
// {
// 	u32 count_;
// 	u32 capacity_;
// 	T * data_;
// };

// template<typename T, typename TAllocator>
// Array<T> allocate_array(u32 count, TAllocator * allocator)
// {
// 	return {};
// }

// void test()
// {

// 	MemoryArena allocator;	
// 	Array<int> array  = allocate_array<int>(6, &allocator);
// }