/*=============================================================================
Leo Tamminen

SmallString can hold short length strings in stack.
=============================================================================*/

struct SmallString
{
	SmallString () = default;
	SmallString (char const *);
	SmallString & operator = (char const *);

	char const * data() const;
	s32 length() const;

	void append(SmallString const &);
	void append(char const *);

	static constexpr s32 max_length();

private:
	/* Note(Leo): Array can hold maxLength characters, 1 null terminator
	and 1 char for length of string */
	static constexpr u32 maxLength = 22;
	char data_ [maxLength + 2];
};

namespace small_string_operators
{
	bool operator == (SmallString const &, char const *);
	bool operator == (char const *, SmallString const &);
	bool operator == (SmallString const &, SmallString const &);
	
	std::ostream & operator << (std::ostream &, SmallString const &);
}

using namespace small_string_operators;

static_assert(std::is_trivial_v<SmallString>, "SmallString is not trivial");
static_assert(std::is_standard_layout_v<SmallString>, "SmallString is not standard layout");

