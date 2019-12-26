/*=============================================================================
Leo Tamminen
shophorn @ github

Handle is kinda like super lazy smart pointer, or not a smart one at all. It
merely holds a reference to an object in a specially reserved container 'storage'.
=============================================================================*/

template<typename T>
struct Handle
{
	int64 _index = -1;

	T * operator->()
	{ 
		MAZEGAME_ASSERT(_index > -1, "Cannot reference uninitialized Handle.");
		return &storage[_index];
	}

	const T * operator -> () const
	{
		MAZEGAME_ASSERT(_index > -1, "Cannot reference uninitialized Handle.");
		return &storage[_index];	
	}

	inline const static Handle Null = {};

	// This is not necessarily the best idea
	inline global_variable ArenaArray<T> storage;
};

template<typename T>
internal bool32
is_handle_valid(Handle<T> handle)
{
	bool32 result = (handle._index > -1) && (handle._index < Handle<T>::storage.count());
	return result;	
}

template<typename T>
internal Handle<T>
make_handle(T item)
{
	/* Note(Leo): We use concrete item instead of constructor arguments
	and (relay/depend/what is the word??) on copy elision to remove copy */
	Handle<T> result = {};
	result._index = push_one(&Handle<T>::storage, item);
	return result;
}

template<typename T>
internal void
allocate_for_handle(MemoryArena * memoryArena, uint64 count)
{
	Handle<T>::storage = push_array<T>(memoryArena, count, false);
}