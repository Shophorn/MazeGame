/*
Leo Tamminen

Common essentials used all around in Friendsimulator project
*/

#if !defined FS_STANDARD_TYPES_H

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

// Todo(Leo): put these in headers
struct v2
{	
	f32 x, y;
};

union v3
{
	struct { f32 x, y, z; };
	struct { f32 r, g, b; };
	struct { v2 xy; f32 ignored_; };
};

union v4
{
	struct { f32 x, y, z, w; };
	struct
	{
		v3 xyz;
		f32 ignored_1;
	};

	struct
	{
		v2 xy, zw;
	};

	struct { f32 r, g, b, a; };
	struct
	{
		v3 rgb; 
		f32 ignored_2;
	};
};

struct quaternion
{
	union
	{
		v3 vector;
		struct {f32 x, y, z; };
	};
	f32 w;
};

struct m44
{
	v4 columns [4];

	// Todo(Leo): These SUPER definetly do not belong here
	v4 & operator [] (s32 column)
	{ 
		return columns[column];
	}

	v4 operator [] (s32 column) const
	{ 
		return columns[column];
	}
};

#define FS_STANDARD_TYPES_H
#endif