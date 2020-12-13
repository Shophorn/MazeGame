/*
Leo Tamminen

String, tah-daa

TODO(LEO): I had some mental designual issues with generic append and parse functions, so I ended up 
	writing them non generic but also added generic interface for serialization and format needs.
	Think this throgh, if issues arise.
	
*/

#include <cstdio> // snprintf

struct String
{
	s64 	length;
	char * 	memory;

	char & operator [] (s32 index) { return memory[index]; }
	char operator [] (s32 index) const { return memory[index]; }

	char * begin() { return memory; }
	char * end() { return memory + length; }
};

// Todo(Leo): This is "dangerous", we are casting the const away :) Let's just be careful
// and not modify contents of this
constexpr String from_cstring(char const * cstring)
{
	String result = {cstring_length(cstring), (char*)cstring};
	return result;
}


internal void reset_string(String & string, s32 capacity)
{
	memory_set(string.memory, 0, capacity);
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
	while(string.length > 0 && is_whitespace_character(string[string.length - 1]))
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

	String result = { length, source.memory };
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

	String result = {length, source.memory};
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


internal void string_parse(String string, f32 * outValue)
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

internal void string_parse(String string, s32 * outValue)
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

internal void string_append_string(String & string, s32 capacity, String other)
{
	Assert(capacity - string.length >= other.length);
	memory_copy(&string[string.length], other.memory, other.length);
	string.length += other.length;
}

internal void string_append_cstring(String & string, s32 capacity, char const * cstring)
{	
	s32 length = cstring_length(cstring);
	string_append_string(string, capacity, from_cstring(cstring));
}

static void string_append_f32(String & string, s64 capacity, f32 value)
{
	s64 length 		= snprintf(string.end(), capacity - string.length, "%f", value);
	string.length 	+= length;
}

static void string_append_s32(String & string, s64 capacity, s32 value)
{
	s64 length 		= snprintf(string.end(), capacity - string.length, "%i", value);
	string.length 	+= length;	
}

static void string_append_s64(String & string, s64 capacity, s64 value)
{
	s64 length 		= snprintf(string.end(), capacity - string.length, "%lli", value);
	string.length 	+= length;
}

internal void string_append_char(String & string, s32 capacity, char character)
{
	string_append_string(string, capacity, {1, &character});
}

#define GENERIC_STRING_APPEND(type, func) internal void string_append(String & string, s32 capacity, type value) { func(string, capacity, value); }

GENERIC_STRING_APPEND(String, string_append_string);
GENERIC_STRING_APPEND(char const *, string_append_cstring);
GENERIC_STRING_APPEND(f32, string_append_f32);
GENERIC_STRING_APPEND(s32, string_append_s32);
GENERIC_STRING_APPEND(s64, string_append_s64);
GENERIC_STRING_APPEND(char, string_append_char);

#undef GENERIC_STRING_APPEND

template<typename TFirst, typename ... TOthers>
internal void string_append_format(String & string, s32 capacity, TFirst first, TOthers ... others)
{
	string_append(string, capacity, first);

	if constexpr (sizeof... (others) > 0)
	{
		string_append_format(string, capacity, others...);
	}
}

