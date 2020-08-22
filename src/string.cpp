struct String
{
	s32 	length;
	char * 	memory;
};

LogInput & operator << (LogInput & log, String const & string)
{
	for (s32 i = 0; i < string.length; ++i)
	{
		log << string.memory[i];
	}

	return log;
}

internal bool is_whitespace_character(char character)
{
	bool result = character == ' ' || character == '\t';
	return result;
}

internal bool is_number_character(char character)
{
	bool result = character >= '0' && character <= '9';
	return result;
}

inline internal char string_first_character(String string)
{
	return string.memory[0];
}

inline internal char string_last_character(String string)
{
	return string.memory[string.length - 1];
}

internal void string_eat_leading_spaces(String & string)
{
	while(string.length > 0 && is_whitespace_character(string_first_character(string)))
	{
		string.memory += 1;
		string.length -= 1;
	}
}

internal void string_eat_following_spaces(String & string)
{
	while(string.length > 0 && is_whitespace_character(string_last_character(string)))
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
	char first = string.memory[0];

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

		if (source.memory[length] == '\r' && source.memory[length + 1] == '\n')
		{
			skipLength = 2;
			break;
		}

		if (source.memory[length] == '\n')
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

		if (source.memory[length] == delimiter)
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

internal bool32 string_equals(String string, char const * cstring)
{
	while(true)
	{
		if (string.length == 0 && *cstring == 0)
		{
			return true;
		}

		if (string.memory[0] != *cstring)
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

internal f32 string_parse_f32(String string)
{
	// Note(Leo): We get 'string' as a copy, since it is just int and a pointer, so we can modify it
	// Todo(Leo): prevent modifying contents though
	f32 value = 0;
	f32 sign = 0;

	if (string.memory[0] == '-')
	{
		sign = -1;
		string_eat_first_character(string);
	}
	else
	{
		sign = 1;
	}

	while(string.length > 0 && string.memory[0] != '.')
	{
		char current = string_eat_first_character(string);
		Assert(is_number_character(current));

		f32 currentValue 	= current - '0';
		value 				*= 10;
		value 				+= currentValue;

	}

	if (string.length == 0)
	{
		return value;
	}

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

	return sign * value;
}

internal void string_append_cstring(String & string, char const * cstring, s32 capacity)
{
	while(string.length < capacity && *cstring != 0)
	{
		string.memory[string.length] = *cstring;

		string.length 	+= 1;
		cstring 		+= 1;
	}
}

internal void string_append_f32(String & string, f32 value, s32 capacity)
{	
	char digits[11] = {};

	// Todo(Leo): check that range is valid

	f32 sign = value < 0 ? -1 : 1;

	value = abs_f32(value);

	s32 exponent = 9;

	if (value < 1)
	{
		exponent = -1;
		digits[0] = '0';
	}
	else
	{
		while(true)
		{
			s32 digit = ((s32)(value / pow_f32(10, exponent))) % 10;
			exponent -= 1;

			if (digit > 0)
			{
				digits[0] = '0' + (char)digit;
				break;
			}
		}
	}

	for(s32 i = 1; i < 11; ++i)
	{
		if (exponent == -1)
		{
			digits[i] = '.';
			++i;
		}

		s32 digit = ((s32)(value / pow_f32(10, exponent))) % 10; 
		digits[i] = '0' + (char)digit;
		exponent -= 1;
	}

	if (sign < 0)
	{
		string.memory[string.length++] = '-';
	}

	for(s32 i = 0; i < 11; ++i)
	{
		string.memory[string.length++] = digits[i];
	}
}

template<typename T>
internal void string_append_format(String & string, s32 capacity, T thing)
{
	if constexpr (std::is_same_v<T, f32>)
	{
		string_append_f32(string, thing, capacity);
	}
	
	if constexpr (std::is_same_v<T, char const *>)
	{
		string_append_cstring(string, thing, capacity);
	}	
}

template<typename TFirst, typename ... TOthers>
internal void string_append_format(String & string, s32 capacity, TFirst first, TOthers ... others)
{
	string_append_format(string, capacity, first);
	string_append_format(string, capacity, others...);
}