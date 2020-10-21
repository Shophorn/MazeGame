/*
Leo Tamminen

Lol, I think languages are supposed to have standard libraries.
*/

#if !defined FS_STANDARD_LIBRARY_H

#include <type_traits>

constexpr f32 common_almost_zero 	= 0.00001f;
constexpr f32 common_almost_one 	= 0.99999f;

/// SIZE MODIFIERS
/* Todo(Leo): Study memory sizes around, maybe these should be actual multiples of 1000
This seems to be just a Windows convention, correct? */
constexpr u64 kilobytes 	(u64 amount) { return 1024 * amount; }
constexpr u64 megabytes 	(u64 amount) { return 1024 * kilobytes(amount); }
constexpr u64 gigabytes 	(u64 amount) { return 1024 * megabytes(amount); }
constexpr u64 terabytes 	(u64 amount) { return 1024 * gigabytes(amount); }

f32 reverse_kilobytes (u64 size) { return size / 1024.0f; }
f32 reverse_megabytes (u64 size) { return reverse_kilobytes(size) / 1024.0f; }
f32 reverse_gigabytes (u64 size) { return reverse_megabytes(size) / 1024.0f; }
f32 reverse_terabytes (u64 size) { return reverse_gigabytes(size) / 1024.0f; }

template<typename _, u32 Count>
constexpr u32 array_count(const _ (&array)[Count])
{
    return Count;
}

#if FS_DEVELOPMENT
	#define Assert(expr) if(!(expr)) { log_assert(__FILE__, __LINE__, nullptr, #expr);		abort(); }
	#define AssertMsg(expr, msg) if(!(expr)) { log_assert(__FILE__, __LINE__, msg, #expr); 	abort(); }
	// Note(Leo): Some things need to asserted in production too, this is a reminder for those only.
	#define AssertRelease AssertMsg

	void log_assert(char const * filename, int line, char const * message, char const * expression);

#else
	#define Assert(expr) 			((void)0);
	#define AssertMsg(expr, msg)	((void)0);
	#define AssertRelease(expr, msg)((void)0);

#endif


// Note(Leo): These use standard math for now. Actually implement these ourself, for the kicks of course.
#include <cmath>
#include <limits>

#include "Memory.cpp"
#include "Math.cpp"

#include "CStringUtility.cpp"
#include "string.cpp"
#include "meta.cpp"

#include "Vectors.cpp"
#include "Quaternion.cpp"
#include "Matrices.cpp"

#include "array.cpp"
#include "serialization.cpp"

#define FS_STANDARD_LIBRARY_H
#endif