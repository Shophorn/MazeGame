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
round_to<s32, float>(float value)
{
    if (value < 0.0f)
        value -= 0.5f;
    else
        value += 0.5f;
    return static_cast<s32>(value);
}

template<> u32
round_to<u32, float>(float value)
{
    value += 0.5f;
    return static_cast<u32>(value);
}


#include<limits>
namespace math
{   
    internal float
    square_root(float value)
    {
        float result = sqrtf(value);
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


    s32 ceil_to_s32(float value)
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
        return value < 0 ? -value : value;
    }

    bool close_enough_small (float a, float b)
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


inline float modulo(float dividend, float divisor)
{
    float result = fmodf(dividend, divisor);
    return result;
}


// Todo(Leo): Add namespace
inline float sine (float value)
{
    float result = sinf(value);
    return result;
}

inline float cosine(float value)
{
    float result = cosf(value);
    return result;
}

inline float Tan(float value)
{
    float result = tanf(value);
    return result;
}

inline float arc_sine(float value)
{
    float result = asinf(value);
    return result;
}

inline float arc_cosine(float value)
{
    float result = acosf(value);
    return result;
}

inline float arc_tan_2(float cosValue, float sinValue)
{
    float result = atan2f(cosValue, sinValue);
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

internal inline constexpr float pi = 3.141592653589793f;
internal inline constexpr float tau = 2 * pi;

constexpr inline float to_degrees(float radians)
{
    constexpr float conversion = 180.0f / pi;
    return conversion * radians;
}

constexpr inline float to_radians(float degrees)
{
    constexpr float conversion = pi / 180.0f;
    return conversion * degrees;
}

inline float interpolate(float from, float to, float t)
{
    float result = (1.0f - t) * from + t * to;
    return result;
}

