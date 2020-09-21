// /*=============================================================================
// Leo Tamminen

// SmallString can hold short length strings in stack.
// =============================================================================*/

// #pragma once

// struct SmallString
// {
// 	SmallString () = default;
// 	SmallString (char const *);
// 	SmallString & operator = (char const *);

// 	char const * data() const;
// 	s32 length() const;

// 	SmallString & append(SmallString const &);
// 	SmallString & append(char const *);

// 	/* Note(Leo): SmallString uses fixed array to store a short string.
// 	Second last character is reserved for null terminator to work well
// 	with cstrings, and the last is used to store length. */
// 	static constexpr s32 arraySize = 24;
// 	static constexpr s32 maxLength = arraySize - 2;
// 	static constexpr s32 lengthPosition = arraySize - 1;

// 	char & operator [](s32 index);

// private:
// 	/* Note(Leo): Array can hold maxLength characters, 1 null terminator
// 	and 1 char for length of string */
// 	char data_ [arraySize];
// };

// namespace small_string_operators
// {
// 	bool operator == (SmallString const &, char const *);
// 	bool operator == (char const *, SmallString const &);
// 	bool operator == (SmallString const &, SmallString const &);
	
// 	std::ostream & operator << (std::ostream &, SmallString const &);
// }

// using namespace small_string_operators;

// static_assert(std::is_trivial_v<SmallString>, "SmallString is not trivial");
// static_assert(std::is_standard_layout_v<SmallString>, "SmallString is not standard layout");

// SmallString make_time_stamp(int hours, int minutes, int seconds);
