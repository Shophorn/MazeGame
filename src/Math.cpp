// TODO(Leo): Proper unit testing!!
// Note(Leo): For now. Actually implement these ourself, for the kicks of course.
#include <cmath>
#include <type_traits>

internal float
Root2(float value)
{
    float result = sqrt(value);
    return result; 
}

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
    /// NUMERIC LIMITS
    template<typename T> constexpr T lowest_value = std::numeric_limits<T>::lowest();
    template<typename T> constexpr T highest_value = std::numeric_limits<T>::max();

    template<typename S>
    constexpr internal S 
    min(S a, S b)
    {
        return (a < b) ? a : b;
    }

    template<typename S>
    constexpr internal S 
    max(S a, S b)
    {
        return (a > b) ? a : b;
    }
}

template<typename S>
internal S
Abs(S num)
{
    if (num < 0)
        return -num;
    return num;
}

template<typename S>
internal S
clamp(S value, S min, S max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
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

inline float to_degrees(float radians)
{
    constexpr float conversion = 180.0f / pi;
    return conversion * radians;
}

inline float to_radians(float degrees)
{
    constexpr float conversion = pi / 180.0f;
    return conversion * degrees;
}

inline float interpolate(float from, float to, float t)
{
    float result = (1.0f - t) * from + t * to;
    return result;
}

