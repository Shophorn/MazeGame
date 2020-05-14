internal s32
compare_cstring(char const * lhs, char const * rhs)
{
	while(*lhs != 0 and *rhs != 0 and *lhs == *rhs)
	{
		++lhs;
		++rhs;
	}

	if (*lhs == *rhs)
		return 0;

	if (*lhs < *rhs)
		return -1;

	if (*lhs >  *rhs)
		return 1;


	Assert(false);

	return 0;
}

internal bool
cstring_equals(char const * a, char const * b)
{
	return compare_cstring(a, b) == 0;
}


internal bool
cstring_begins_with(char const * cstring, char const * begin)
{
	while(*begin != 0)
	{
		if (*cstring == 0 || *cstring != *begin)
		{
			return false;
		}

		++cstring;
		++begin;
	}

	return true;
}

template<s32 Capacity = 128>
struct CStringBuilder
{
	char cstring [Capacity];
	s32 length = 0;

	CStringBuilder() = default;

	CStringBuilder(char const * string)
	{
		copy_cstring(string);
	}

	CStringBuilder & operator + (char const * string)
	{
		copy_cstring(string);
		return *this;
	}

	CStringBuilder & operator + (f32 value)
	{
		std::stringstream ss;
		ss << value;
		std::string s = ss.str();
		char const * cstring = s.c_str();
		copy_cstring(cstring);
		return *this;
	}


	operator const char * ()
	{
		return cstring;
	}

	void copy_cstring(char const * source)
	{
		while(*source != 0 && length < Capacity)
		{
			cstring[length] = *source;
			source++;
			length++;
		}
		cstring[length] = 0;
	}
};

// template <s32 C>
// CStringBuilder<C> & operator + (CStringBuilder<C> & builder, f32 value)
// {
// 	std::strinstream ss (value);
// 	builder + ss.str().c_str();
// 	return builder;
// }


template<s32 Capacity>
std::ostream & operator << (std::ostream & os, CStringBuilder<Capacity> builder)
{
	os << builder.cstring;
	return os;
}
