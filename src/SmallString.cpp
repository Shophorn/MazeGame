#include "SmallString.hpp"


namespace small_string_internal_
{
	/* Note(Leo): This is magical function that mgically does everything.
	It copies contents and secretly sets null terminator at the end of
	content and finally sets current length to last position. */
	internal void
	copy_cstring(char * destination, u32 startPosition, char const * source)
	{
		u32 srcIndex = 0;
		u32 dstIndex = startPosition;

		while (source[srcIndex] != 0 and dstIndex < SmallString::maxLength)
		{
			destination[dstIndex] = source[srcIndex];
			++dstIndex;
			++srcIndex;
		}

		destination[dstIndex] = 0;
		destination[SmallString::lengthPosition] = dstIndex;
	}
	
}


char const * SmallString::data() const { return data_; }

s32 SmallString::length() const { return data_[lengthPosition]; }

char & SmallString::operator[](s32 index)
{
	Assert(index < maxLength);
	return data_[index];
}

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

SmallString & SmallString::append(SmallString const & source)
{
	small_string_internal_::copy_cstring(data_, length(), source.data_);
	return *this;
}

SmallString & SmallString::append(char const * source)
{
	small_string_internal_::copy_cstring(data_, length(), source);
	return *this;
}

// ----------------------------------------------------------------------------

bool small_string_operators::operator == (SmallString const & s, char const * c)
{
	return compare_cstring(s.data(), c) == 0;
}

bool small_string_operators::operator == (char const * c, SmallString const & s)
{
	return compare_cstring(c, s.data()) == 0;
}

bool small_string_operators::operator == (SmallString const & lhs, SmallString const & rhs)
{
	return compare_cstring(lhs.data(), rhs.data()) == 0;
}

std::ostream & small_string_operators::operator << (std::ostream & os, SmallString const & string)
{
	os << string.data();
	return os;
}

// ----------------------------------------------------------------------------

SmallString make_timestamp(s32 hours, s32 minutes, s32 seconds)
{
	SmallString timestamp = "XX_XX_XX";
	timestamp[0] = '0' + ((hours / 10) % 24);
	timestamp[1] = '0' + (hours % 10);
	// skip 2
	timestamp[3] = '0' + ((minutes / 10) % 60);
	timestamp[4] = '0' + (minutes % 10);
	// skip 5
	timestamp[6] = '0' + ((seconds / 10) % 60);
	timestamp[7] = '0' + (seconds % 10);
	return timestamp;
}