/*
Leo Tamminen

Common essentials used all around in Friendsimulator project
*/

#if !defined FS_ESSENTIALS_HPP




// Todo(Leo): Find where these come from
#undef min
#undef max

// SEARCHABILITY BOOST
#define local_persist static
#define global_variable static

// Todo(Leo): this might not e good idea... Or good name anyway, some libraries seem to use it internaly (rapidjson at least)
#define internal static

/// SENSIBLE SIMPLE TYPES
// Todo(Leo): These should come from platform layer, since that is where they are defined anyway
using s8    = signed char;
using s16   = signed short;
using s32   = signed int;
using s64   = signed long long;

static_assert(sizeof(s8) == 1, "Invalid type alias for 's8'");
static_assert(sizeof(s16) == 2, "Invalid type alias for 's16'");
static_assert(sizeof(s32) == 4, "Invalid type alias for 's32'");
static_assert(sizeof(s64) == 8, "Invalid type alias for 's64'");

using u8    = unsigned char;
using u16   = unsigned short;
using u32   = unsigned int;
using u64   = unsigned long long;

static_assert(sizeof(u8) == 1, "Invalid type alias for 'u8'");
static_assert(sizeof(u16) == 2, "Invalid type alias for 'u16'");
static_assert(sizeof(u32) == 4, "Invalid type alias for 'u32'");
static_assert(sizeof(u64) == 8, "Invalid type alias for 's64'");

using bool8 = s8;
using bool32 = s32;

using wchar = wchar_t;

/* Note(Leo): Not super sure about these, they are currently not used consitently
around codebase, but I left them where they were due to inability to make a decision */
using byte = u8;
using f32 = float;
using f64 = double;

// Todo(Leo): Study Are there any other possibilities than these always being fixed
static_assert(sizeof(f32) == 4, "Invalid type alias for 'f32'");
static_assert(sizeof(f64) == 8, "Invalid type alias for 'f64'");

/// SIZE MODIFIERS
/* Todo(Leo): Study memory sizes around, maybe these should be actual multiples of 1000
This seems to be just a Windows convention, correct? */
constexpr u64 kilobytes 	(u64 amount) { return 1024 * amount; }
constexpr u64 megabytes 	(u64 amount) { return 1024 * kilobytes(amount); }
constexpr u64 gigabytes 	(u64 amount) { return 1024 * megabytes(amount); }
constexpr u64 terabytes 	(u64 amount) { return 1024 * gigabytes(amount); }

template<typename _, u32 Count>
constexpr u32 array_count(const _ (&array)[Count])
{
    return Count;
}

#if MAZEGAME_DEVELOPMENT
	#define Assert(expr) if(!(expr)) { log_assert(__FILE__, __LINE__, nullptr, #expr); 			abort(); }
	#define AssertMsg(expr, msg) if(!(expr)) { log_assert(__FILE__, __LINE__, msg, #expr); 	abort(); }
	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define AssertRelease AssertMsg

	void log_assert(char const * filename, int line, char const * message, char const * expression);

#else
	#define Assert(expr) 			((void)0);
	#define AssertMsg(expr, msg)	((void)0);
	#define AssertRelease 			((void)0);

#endif


#include "Math.cpp"
#include "Memory.cpp"

#include "CStringUtility.cpp"
#include "string.cpp"
#include "meta.cpp"
#include "SmallString.cpp"

#include "Logging.cpp"

#include "Vectors.cpp"
#include "Quaternion.cpp"
#include "Matrices.cpp"


#include "serialization.cpp"
#include "array2.cpp"

#define FS_ESSENTIALS_HPP
#endif