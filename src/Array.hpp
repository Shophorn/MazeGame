// TODO(Leo): This was early mockup, do somethin useful or REMOVE altogether

#ifndef ARRAY_HPP

template <typename Type, int Count>
struct Array
{
	using value_type = Type;
	
	value_type values [Count];

	value_type & operator [] (uint32 index)
	{
		// Todo(Leo): Assert index okaybility.
		return values[index];
	}

	uint32 count() { return Count; }	
};

// template <typename Type, int Count, typename IndexType = uint64>
// struct Array
// {
// 	using data_type = Type;
// 	using index_type = IndexType;

// 	data_type data [Count];

// 	data_type & operator [] (index_type index)
// 	{
// 		// Todo(Leo): Assert index okaybility.
// 		return data[index];
// 	}

// 	uint32 count() { return Count; }	
// };

#define ARRAY_HPP
#endif


