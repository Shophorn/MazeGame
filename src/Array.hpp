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

#define ARRAY_HPP
#endif