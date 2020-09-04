/*
Leo Tamminen

String, tah-daa

TODO(LEO): I had some mental designual issues with generic append and parse functions, so I ended up 
	writing them non generic but also added generic interface for serialization and format needs.
	Think this throgh, if issues arise.
*/

struct String
{
	s32 	length;
	char * 	memory;

	char & last() { return memory[length - 1]; }

	char & operator [] (s32 index) { return memory[index]; }
	char operator [] (s32 index) const { return memory[index]; }

	char * begin() { return memory; }
	char * end() { return memory + length; }

	char const * begin() const { return memory; }
	char const * end() const { return memory + length; }
};

LogInput & operator << (LogInput & log, String const & string)
{
	for (s32 i = 0; i < string.length; ++i)
	{
		log << string[i];
	}

	return log;
}

internal void reset_string(String & string, s32 capacity)
{
	fill_memory(string.memory, 0, capacity);
	string.length = 0;
}

internal bool is_whitespace_character(char character)
{
	bool result = character == ' ' || character == '\t';
	return result;
}

internal bool is_newline_character(char character)
{
	bool result = character == '\r' || character == '\n';
	return result;
}

internal bool is_number_character(char character)
{
	bool result = character >= '0' && character <= '9';
	return result;
}

internal void string_eat_leading_spaces_and_lines(String & string)
{
	while(string.length > 0 && (is_whitespace_character(string[0]) || is_newline_character(string[0])))
	{
		string.memory += 1;
		string.length -= 1;
	}
}

internal void string_eat_leading_spaces(String & string)
{
	while(string.length > 0 && is_whitespace_character(string[0]))
	{
		string.memory += 1;
		string.length -= 1;
	}
}

internal void string_eat_following_spaces(String & string)
{
	while(string.length > 0 && is_whitespace_character(string.last()))
	{
		string.length -= 1;
	}
}

internal void string_eat_spaces(String & string)
{
	string_eat_leading_spaces(string);
	string_eat_following_spaces(string);
}

internal char string_eat_first_character(String & string)
{
	char first = string[0];

	string.memory += 1;
	string.length -= 1;

	return first;
}

String string_extract_line(String & source)
{
	s32 length 		= 0;
	s32 skipLength 	= 0;
	while(true)
	{
		if (length == source.length)
		{
			break;
		}

		if (source[length] == '\r' && source[length + 1] == '\n')
		{
			skipLength = 2;
			break;
		}

		if (source[length] == '\n')
		{
			skipLength = 1;
			break;
		}

		length += 1;
	}

	String result = 
	{
		.length = length,
		.memory = source.memory
	};
	string_eat_spaces(result);

	#ifndef _WIN32
	static_assert(false, "Not sure if newline is still the same on this os");
	#endif

	// Note(Leo) add two for \r and \n
	source.memory += length + skipLength;
	source.length -= length + skipLength;

	return result;
}

String string_extract_until_character(String & source, char delimiter)
{
	s32 length = 0;
	while(true)
	{
		if (length == source.length)
		{
			break;
		}

		if (source[length] == delimiter)
		{
			break;
		}

		length += 1;
	}

	String result =
	{
		.length = length,
		.memory = source.memory,
	};
	string_eat_spaces(result);

	// Note(Leo): Add one for delimiting character
	source.memory += length + 1;
	source.length -= length + 1;

	string_eat_leading_spaces(source);

	return result;
}

// Note(Leo): string is passed by value since it is small, and can therefore be modified in function
internal bool32 string_equals(String string, char const * cstring)
{
	while(true)
	{
		if (string.length == 0 && *cstring == 0)
		{
			return true;
		}

		if (string[0] != *cstring)
		{
			return false;
		}

		if (string.length == 0 && *cstring != 0)
		{
			return false;
		}

		if (string.length > 0 && *cstring == 0)
		{
			return false;
		}

		string.memory += 1;
		string.length -= 1;

		cstring += 1;
	}
}

internal void string_parse_f32(String string, f32 * outValue)
{
	// Note(Leo): We get 'string' as a copy, since it is just int and a pointer, so we can modify it
	// Todo(Leo): prevent modifying contents though
	f32 value = 0;
	f32 sign = 0;

	if (string[0] == '-')
	{
		sign = -1;
		string_eat_first_character(string);
	}
	else
	{
		sign = 1;
	}

	while(string.length > 0 && string[0] != '.')
	{
		char current = string_eat_first_character(string);
		Assert(is_number_character(current));

		f32 currentValue 	= current - '0';
		value 				*= 10;
		value 				+= currentValue;
	}

	if (string.length > 0)
	{
		// Note(Leo): this is decimal point
		string_eat_first_character(string);

		f32 power = 0.1;

		while(string.length > 0)
		{
			char current = string_eat_first_character(string);
			Assert(is_number_character(current));

			f32 currentValue 	= current - '0';
			value 				+= power * currentValue;
			power 				*= 0.1;
		}
	}

	*outValue = sign * value;
}

internal void string_parse_s32(String string, s32 * outValue)
{
	// Note(Leo): We get 'string' as a copy, since it is just int and a pointer, so we can modify it
	// Todo(Leo): prevent modifying contents though
	s32 value = 0;
	s32 sign = 0;

	if (string[0] == '-')
	{
		sign = -1;
		string_eat_first_character(string);
	}
	else
	{
		sign = 1;
	}

	while(string.length > 0)
	{
		char current = string_eat_first_character(string);
		Assert(is_number_character(current));

		f32 currentValue 	= current - '0';
		value 				*= 10;
		value 				+= currentValue;

	}

	*outValue = sign * value;
}

internal void string_parse_v2(String string, v2 * outValue)
{
	String part0 = string_extract_until_character(string, ',');
	String part1 = string;

	string_parse_f32(part0, &outValue->x);
	string_parse_f32(part1, &outValue->y);
}


internal void string_parse_v3(String string, v3 * outValue)
{
	String part0 = string_extract_until_character(string, ',');
	String part1 = string_extract_until_character(string, ',');
	String part2 = string;

	string_parse_f32(part0, &outValue->x);
	string_parse_f32(part1, &outValue->y);
	string_parse_f32(part2, &outValue->z);
}

#define GENERIC_STRING_PARSE(type, func) internal void generic_string_parse(String const & string, type * outValue) {func(string, outValue);}

GENERIC_STRING_PARSE(f32, string_parse_f32);
GENERIC_STRING_PARSE(s32, string_parse_s32);
GENERIC_STRING_PARSE(v2, string_parse_v2);
GENERIC_STRING_PARSE(v3, string_parse_v3);

#undef GENERIC_STRING_PARSE

internal void string_append_string(String & string, s32 capacity, String other)
{
	Assert(capacity - string.length >= other.length);
	copy_memory(string.end(), other.memory, other.length);
	string.length += other.length;
}

internal void string_append_cstring(String & string, s32 capacity, char const * cstring)
{
	// todo(Leo): assert in this or truncate in normal string append, at least do the same thing in both
	while(string.length < capacity && *cstring != 0)
	{
		*string.end() = *cstring;

		string.length 	+= 1;
		cstring 		+= 1;
	}
}

internal void string_append_f32(String & string, s32 capacity, f32 value)
{	
	// Todo(Leo): check that range is valid

	constexpr s32 digitCapacity = 8;

	// Note(Leo): added one is for possible negative sign
	char digits[digitCapacity + 1] = {};
	s32 digitCount = 0;

	fill_memory(digits, '0', digitCapacity);
	Assert((capacity - string.length) > array_count(digits))
	

	if (value < 0)
	{
		digits[digitCount++] = '-';
		value = abs_f32(value);
	}

	constexpr s32 maxExponent 	= 6;
	
	constexpr f32 powers [2 * maxExponent + 1] =
	{
		0.000001,
		0.00001,
		0.0001,
		0.001,
		0.01,
		0.1,
		1,
		10,
		100,
		1000,
		10000,
		100000,
		1000000,
	};

	auto get_digit = [&powers, value](s32 exponent) -> char
	{
		f32 power 	= powers[exponent + maxExponent];
	
		// Note(Leo): Modulo limits upper bound, max limits lower bound;
		s32 digit 	= ((s32)(value / power)) % 10;
		digit 		= max_s32(0, digit);
	
		return '0' + (char)digit;
	};

	s32 exponent 	= maxExponent;

	// Note(Leo): first loop to find first number, so we don't write leading zeros wastefully
	while(exponent >= 0)
	{
		char digit = get_digit(exponent);
		exponent--;

		if (digit != '0')
		{
			digits[0] = digit;
			break;
		}
	}

	digitCount += 1;

	// Note(Leo): second loop to find rest
	while((digitCount < digitCapacity) && (exponent >= -maxExponent))
	{
		if (exponent == -1)
		{
			digits[digitCount] = '.';
			digitCount++;
		}
		
		digits[digitCount] = get_digit(exponent);

		digitCount++;
		exponent--;
	}

	string_append_string(string, capacity, String{digitCount, digits});
}

internal void string_append_s32(String & string, s32 capacity, s32 value)
{	
	// Todo(Leo): check that range is valid

	// Note(Leo): these are connected
	constexpr s32 digitCapacity 	= 8;
	constexpr s32 startMultiplier 	= 10'000'000;

	// Note(Leo): added one is for possible negative sign
	char digits[digitCapacity + 1] = {};
	fill_memory(digits, '0', array_count(digits));
	s32 digitCount = 0;

	Assert((capacity - string.length) > array_count(digits));

	if (value == 0)
	{
		*string.end() = '0';
		string.length += 1;
	}
	else
	{
		if (value < 0)
		{
			value *= -1;
			digits[digitCount++] = '-';
		}

		s32 multiplier 				= startMultiplier;
		bool32 firstNonZeroFound 	= false;

		for (s32 i = 0; i < digitCapacity; ++i)
		{
			s32 valueAtMultiplier = (value / multiplier) % 10;
			multiplier /= 10;

			firstNonZeroFound = firstNonZeroFound || (valueAtMultiplier > 0);

			if (firstNonZeroFound)
			{
				digits[digitCount++] = '0' + valueAtMultiplier;
			}
		}

		string_append_string(string, capacity, {digitCount, digits});
	}
}

internal void string_append_v2(String & string, s32 capacity, v2 value)
{
	string_append_f32(string, capacity, value.x);	
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);	
}

internal void string_append_v3(String & string, s32 capacity, v3 value)
{
	string_append_f32(string, capacity, value.x);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.y);
	string_append_cstring(string, capacity, ", ");
	string_append_f32(string, capacity, value.z);
}


#define GENERIC_STRING_APPEND(type, func) internal void generic_string_append(String & string, s32 capacity, type value) { func(string, capacity, value); }

GENERIC_STRING_APPEND(String, string_append_string);
GENERIC_STRING_APPEND(char const *, string_append_cstring);
GENERIC_STRING_APPEND(f32, string_append_f32);
GENERIC_STRING_APPEND(s32, string_append_s32);
GENERIC_STRING_APPEND(v2, string_append_v2);
GENERIC_STRING_APPEND(v3, string_append_v3);

#undef GENERIC_STRING_APPEND

template<typename TFirst, typename ... TOthers>
internal void string_append_format(String & string, s32 capacity, TFirst first, TOthers ... others)
{
	generic_string_append(string, capacity, first);

	if constexpr (sizeof... (others) > 0)
	{
		string_append_format(string, capacity, others...);
	}
}

