// TODO(Leo): Proper unit testing!!
// Note(Leo): For now. Actually implement these ourself, for the kicks of course.
#include <cmath>
#include <type_traits>


template<typename TTo, typename TFrom>
internal TTo
floor_to(TFrom value)
{
    TTo floorValue = static_cast<TTo>(value);
    if (value < 0)
        floorValue = floorValue -1;
    return static_cast<TTo>(floorValue);
}

template <typename TTo, typename TFrom>
internal TTo round_to(TFrom value);

template<> s32
round_to<s32, f32>(f32 value)
{
    if (value < 0.0f)
        value -= 0.5f;
    else
        value += 0.5f;
    return static_cast<s32>(value);
}

template<> u32
round_to<u32, f32>(f32 value)
{
    value += 0.5f;
    return static_cast<u32>(value);
}

constexpr f32 highest_f32 = std::numeric_limits<f32>::max();
constexpr f32 lowest_f32 = std::numeric_limits<f32>::lowest();

constexpr u32 max_u32 = std::numeric_limits<u32>::max();
constexpr u32 min_u32 = std::numeric_limits<u32>::min();

s32 floor_to_s32(f32 f)
{
    return (s32)floorf(f);
}

#include<limits>
namespace math
{   
    internal f32
    square_root(f32 value)
    {
        f32 result = sqrtf(value);
        return result; 
    }


    /// NUMERIC LIMITS
    template<typename T> constexpr T lowest_value = std::numeric_limits<T>::lowest();
    template<typename T> constexpr T highest_value = std::numeric_limits<T>::max();

    template<typename T> 
    constexpr std::enable_if_t<std::is_floating_point_v<T>, T>closest_to_zero = std::numeric_limits<T>::min();

    template<typename T>
    constexpr internal T min(T a, T b)
    {
        return (a < b) ? a : b;
    }

    template<typename T>
    constexpr internal T max(T a, T b)
    {
        return (a > b) ? a : b;
    }


    s32 ceil_to_s32(f32 value)
    {
        s32 result = (s32)std::ceil(value);
        return result;
    }

    template<typename T>
    internal constexpr T absolute(T value)
    {
        /* Story(Leo): I tested optimizing this using both pointer casting and type punning
        and manually setting the sign bit, and it was significantly faster on some types.
        That was, until I tried setting compiler optimization level up, and then this was
        fastest at least as significantly.

        Always optimize with proper compiler options :) */
        // return value < 0 ? -value : value;
        return abs(value);
    }

    bool close_enough_small (f32 a, f32 b)
    {
        return absolute(a - b) < 0.00001f;
    }

    f32 smooth (f32 v)
    {
        return (3 - 2 * v) * v * v;
    }

    template <typename T>
    internal T clamp(T value, T min, T max)
    {
        value = value < min ? min : value;
        value = value > max ? max : value;
        return value;
    }

    internal s32 loop(s32 value, s32 min, s32 max)
    {
        if (value < min)
        {
            s32 d = min - value;
            value = max - d + 1;
        }
        else if (value > max)
        {
            s32 d = value - max;
            value = min + d - 1;
        }
        return value;
    }

    template<typename T>
    internal bool is_nan(T value)
    { 
        return value != value;
    }

    template<typename T>
    internal T distance(T a, T b)
    {
        return absolute(a - b);
    }

    template<typename T>
    internal inline T pow2(T value)
    {
        return value * value;
    }

}


inline f32 modulo(f32 dividend, f32 divisor)
{
    f32 result = fmodf(dividend, divisor);
    return result;
}


// Todo(Leo): Add namespace
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

template<typename S>
inline S
Sign(S value)
{
    if (value < 0) return -1;
    if (value > 0) return 1;
    return 0;
}

internal inline constexpr f32 pi = 3.141592653589793f;
internal inline constexpr f32 tau = 2 * pi;

constexpr inline f32 to_degrees(f32 radians)
{
    constexpr f32 conversion = 180.0f / pi;
    return conversion * radians;
}

constexpr inline f32 to_radians(f32 degrees)
{
    constexpr f32 conversion = pi / 180.0f;
    return conversion * degrees;
}

inline f32 interpolate(f32 from, f32 to, f32 t)
{
    f32 result = (1.0f - t) * from + t * to;
    return result;
}

