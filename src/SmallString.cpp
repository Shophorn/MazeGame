struct SmallString
{
	static constexpr u32 maxLength = 23;
	static constexpr u32 arrayLength = maxLength + 1;

	SmallString () = default;
	SmallString (char const * source);
	SmallString & operator = (char const * source);

	char const * data() const { return data_; }

private:
	char data_ [arrayLength];

	void assign_char_pointer(char const * source);
};

void SmallString::assign_char_pointer(char const * source)
{
	u32 count = 0;
	while(source[count] != 0 && count < SmallString::maxLength)
	{
		data_[count] = source[count];
		count++;
	}

	// Add null terminator
	data_[count] = 0;	
}

SmallString::SmallString(char const * source)
{
	this->assign_char_pointer(source);
}

SmallString & SmallString::operator = (char const * source)
{
	this->assign_char_pointer(source);
	return *this;
}

internal s32 compare_char_array(char const * lhs, char const * rhs)
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


bool operator == (SmallString const & s, char const * c)
{
	return compare_char_array(s.data(), c) == 0;
}

bool operator == (char const * c, SmallString const & s)
{
	return compare_char_array(c, s.data()) == 0;
}

bool operator == (SmallString const & lhs, SmallString const & rhs)
{
	return compare_char_array(lhs.data(), rhs.data()) == 0;
}


std::ostream & operator << (std::ostream & os, SmallString const & string)
{
	os << string.data();
	return os;
}

static_assert(std::is_trivial_v<SmallString>, "SmallString is not trivial");
static_assert(std::is_standard_layout_v<SmallString>, "SmallString is not standard_layout");
