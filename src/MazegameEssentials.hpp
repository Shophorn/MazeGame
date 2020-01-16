/*=============================================================================
Leo Tamminen

Common essentials used all around in :MAZEGAME: project
===============================================================================*/

#if !defined MAZEGAME_ESSENTIALS_HPP

// SEARCHABILITY BOOST
#define class_member static
#define local_persist static

// Todo(Leo): this might not e good idea... Or good name anyway, some libraries seem to use it internaly (rapidjson at least)
#define internal static
#define global_variable static

#define ARRAY_COUNT(array) sizeof(array) / sizeof((array)[0])

/// SENSIBLE SIMPLE TYPES
// Todo(Leo): These should come from platform layer, since that is where they are defined anyway
using int8 = signed char;
using int16 = signed short;
using int32 = signed int;
using int64 = signed long long;

static_assert(sizeof(int8) == 1, "Invalid type alias for int8");
static_assert(sizeof(int16) == 2, "Invalid type alias for int16");
static_assert(sizeof(int32) == 4, "Invalid type alias for int32");
static_assert(sizeof(int64) == 8, "Invalid type alias for int64");

using uint8 = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

static_assert(sizeof(uint8) == 1, "Invalid type alias for uint8");
static_assert(sizeof(uint16) == 2, "Invalid type alias for uint16");
static_assert(sizeof(uint32) == 4, "Invalid type alias for uint32");
static_assert(sizeof(uint64) == 8, "Invalid type alias for int64");

using bool8 = int8;
using bool32 = int32;

using wchar = wchar_t;
using byte = uint8;

/* Note(Leo): Not super sure about these, they are currently not used consitently
around codebase, but I left them where they were due to inability to make a decision */
using real32 = float;
using real64 = double;

// Todo(Leo): Study Are there any other possibilities than these always being fixed
static_assert(sizeof(real32) == 4, "Invalid type alias for real32");
static_assert(sizeof(real64) == 8, "Invalid type alias for real64");

/// NUMERIC LIMITS
#include<limits>
template <typename Number>
static constexpr Number MinValue = std::numeric_limits<Number>::min();

template <typename Number>
static constexpr Number MaxValue = std::numeric_limits<Number>::max();

/// SIZE MODIFIERS
/* Todo(Leo): Study memory sizes around, maybe these should be actual multiples of 1000
This seems to be just a Windows convention, correct? */
constexpr uint64 Kilobytes 	(uint64 amount) { return 1024 * amount; }
constexpr uint64 Megabytes 	(uint64 amount) { return 1024 * Kilobytes(amount); }
constexpr uint64 Gigabytes 	(uint64 amount) { return 1024 * Megabytes(amount); }
constexpr uint64 Terabytes 	(uint64 amount) { return 1024 * Gigabytes(amount); }

inline uint64
align_up_to(uint64 alignment, uint64 size)
{
    /*
    Divide to get number of full multiples of alignment
    Add one so we end up bigger
    Multiply to get actual size, that is both bigger AND a multiple of alignment
    */

    size /= alignment;
    size += 1;
    size *= alignment;
    return size;
}

const char * 
bool_str(bool value)
{
	return (value ? "True" : "False");
}

template<typename T>
uint64 get_size_of()
{ 
    return sizeof(T);
}

#define MAZEGAME_ESSENTIALS_HPP
#endif