/*=============================================================================
Leo Tamminen
shophorn @ github
=============================================================================*/

template<typename T>
struct Handle
{
	/* Note(Leo): we will store various metadata in 'thing_' later, such as
	validity, container index, generation etc. We can manage with u32's max value
	amount of items, and this will still be just a size of pointer. */
	u32 thing_;
	u32 index_;

	T * operator->()
	{ 
		DEBUG_ASSERT(thing_ > 0, "Cannot reference uninitialized Handle.");
		return &storage[index_];
	}

	operator T * ()
	{
		DEBUG_ASSERT(thing_ > 0, "Cannot reference uninitialized Handle.");
		return &storage[index_];	
	}

	/* Todo(Leo): this is bad
		1. doesn't work with multiple scenes/allocations
		2. when we reload game, this is lost, and stops working -> prevents hot reloading
	*/
	inline global_variable ArenaArray<T> storage;
};


template<typename T>
internal bool32
is_handle_valid(Handle<T> handle)
{
	bool32 result = (handle.thing_ > 0) && (handle.index_ < Handle<T>::storage.count());
	return result;	
}

template<typename T>
internal Handle<T>
make_handle(T item)
{
	/* Note(Leo): 	We use concrete item instead of constructor arguments
					and rely on copy elision to remove copy.

					However, I am not sure if it works that way.*/
	Handle<T> result = {};
	result.thing_ = 1;
	result.index_ = push_one(Handle<T>::storage, item);
	return result;
}

template<typename T>
internal void
allocate_for_handle(MemoryArena * memoryArena, u32 count)
{
	Handle<T>::storage = reserve_array<T>(memoryArena, count);
}