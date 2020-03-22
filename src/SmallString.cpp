#include "SmallString.hpp"

namespace small_string_internal_
{
	internal void
	copy_cstring(char * destination, u32 startPosition, char const * source)
	{
		u32 srcIndex = 0;
		u32 dstIndex = startPosition;

		while (source[srcIndex] != 0 and dstIndex < SmallString::max_length())
		{
			destination[dstIndex] = source[srcIndex];
			++dstIndex;
			++srcIndex;
		}

		destination[dstIndex] = 0;
		destination[SmallString::max_length() - 1] = dstIndex;
	}

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


		assert(false);

		return 0;
	}
}

// ----------------------------------------------------------------------------

char const * SmallString::data() const { return data_; }

s32 SmallString::length() const { return data_[maxLength -1]; }

constexpr s32 SmallString::max_length() { return maxLength; }

// ----------------------------------------------------------------------------

SmallString::SmallString(char const * source)
{ 
	small_string_internal_::copy_cstring(data_, 0, source);
}

SmallString & SmallString::operator = (char const * source)
{
	small_string_internal_::copy_cstring(data_, 0, source);
	return *this;
}

void SmallString::append(SmallString const & source)
{
	small_string_internal_::copy_cstring(data_, length(), source.data_);
}

void SmallString::append(char const * source)
{
	small_string_internal_::copy_cstring(data_, length(), source);
}

// ----------------------------------------------------------------------------

bool small_string_operators::operator == (SmallString const & s, char const * c)
{
	return small_string_internal_::compare_cstring(s.data(), c) == 0;
}

bool small_string_operators::operator == (char const * c, SmallString const & s)
{
	return small_string_internal_::compare_cstring(c, s.data()) == 0;
}

bool small_string_operators::operator == (SmallString const & lhs, SmallString const & rhs)
{
	return small_string_internal_::compare_cstring(lhs.data(), rhs.data()) == 0;
}

std::ostream & small_string_operators::operator << (std::ostream & os, SmallString const & string)
{
	os << string.data();
	return os;
}