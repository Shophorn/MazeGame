// TODO(Leo): Proper unit testing!!
// Note(Leo): Thesee use standard math for now. Actually implement these ourself, for the kicks of course.
#include <cmath>

#include <limits>
constexpr f32 highest_f32   = std::numeric_limits<f32>::max();
constexpr f32 lowest_f32    = std::numeric_limits<f32>::lowest();
constexpr f32 smallest_f32  = std::numeric_limits<f32>::min();

constexpr s16 max_value_s16 = std::numeric_limits<s16>::max();

constexpr s16 max_value_u16 = std::numeric_limits<u16>::max();
constexpr u32 max_value_u32 = std::numeric_limits<u32>::max();
constexpr u64 max_value_u64 = std::numeric_limits<u64>::max();

inline f32 abs_f32(f32 value)
{
    /* Story(Leo): I tested optimizing this using both pointer casting and type punning
    and manually setting the sign bit, and it was significantly faster on some types.
    That was, until I tried setting compiler optimization level up, and then this was
    fastest at least as significantly.

    Always optimize with proper compiler options :) */
    return fabsf(value);
}


inline f32 mod_f32(f32 dividend, f32 divisor)
{
    f32 result = fmodf(dividend, divisor);
    return result;
}

inline f32 floor_f32(f32 value)
{
    return floorf(value);
}

inline f32 ceil_f32(f32 value)
{
    return ceilf(value);
}

inline f32 round_f32(f32 value)
{
    return roundf(value);
}

inline f32 sign_f32(f32 value)
{
    return copysign(1.0f, value);
}

inline f32 square_f32(f32 value)
{
    return value * value;
}

inline f32 square_root_f32 (f32 value)
{
    return sqrtf(value);
}

inline f32 interpolate(f32 from, f32 to, f32 t)
{
    f32 result = (1.0f - t) * from + t * to;
    return result;
}

f32 min_f32(f32 a, f32 b)
{
    return (a < b) ? a : b;
}

f32 max_f32(f32 a, f32 b)
{
    return (a > b) ? a : b;
}

inline f32 clamp_f32(f32 value, f32 min, f32 max)
{
    value = value < min ? min : value;
    value = value > max ? max : value;
    return value;
}

/// ******************************************************************
/// MATHFUN, fun math stuff :)
f32 mathfun_smooth_f32 (f32 v)
{
    return (3 - 2 * v) * v * v;
}

f32 mathfun_pingpong_f32(f32 value, f32 length)
{
    f32 length2 = 2 * length;
    f32 result  = length - abs_f32((value - floor_f32(value / length2) * length2) - length);
    return result;
}

/// ******************************************************************
/// TRIGONOMETRY

internal constexpr f32 π = 3.141592653589793f;

inline f32 sine (f32 value)
{
    f32 result = sinf(value);
    return result;
}

inline f32 cosine(f32 value)
{
    f32 result = cosf(value);
    return result;
}

inline f32 Tan(f32 value)
{
    f32 result = tanf(value);
    return result;
}

inline f32 arc_sine(f32 value)
{
    f32 result = asinf(value);
    return result;
}

inline f32 arc_cosine(f32 value)
{
    f32 result = acosf(value);
    return result;
}

// Todo(Leo): Study this.
// https://www.quora.com/Why-do-we-not-use-cross-product-instead-of-dot-product-for-finding-out-the-angle-between-the-two-vectors
inline f32 arctan2(f32 y, f32 x)
{
    f32 result = atan2f(y, x);
    return result;
}

// Todo(Leo): remove, and just use radians :)
constexpr inline f32 to_radians(f32 degrees)
{
    constexpr f32 conversion = π / 180.0f;
    return conversion * degrees;
}