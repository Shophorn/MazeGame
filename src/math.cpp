// TODO(Leo): Proper unit testing!!
// Note(Leo): For now. Actually implement these ourself, for the kicks of course.
#include <cmath>
#include <type_traits>

internal real32
Root2(real32 value)
{
    real32 result = sqrt(value);
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

template<typename IntegerType, typename FloatType>
IntegerType
CeilTo(FloatType value)
{
    int64 ceilValue = static_cast<int64>(value);
    if (value > 0)
        ceilValue = ceilValue + 1;
    return static_cast<IntegerType>(ceilValue);
}

template <typename TTo, typename TFrom>
internal TTo round_to(TFrom value);

template<>
internal int32
round_to<int32, float>(float value)
{
    if (value < 0.0f)
        value -= 0.5f;
    else
        value += 0.5f;
    return static_cast<int32>(value);
}

template<>
internal uint32
round_to<uint32, float>(float value)
{
    value += 0.5f;
    return static_cast<uint32>(value);
}



template<typename Number>
internal Number 
Min(Number a, Number b)
{
    return (a < b) ? a : b;
}

template<typename Number>
internal Number 
Max(Number a, Number b)
{
    return (a > b) ? a : b;
}

template<typename Number>
internal Number
Abs(Number num)
{
    if (num < 0)
        return -num;
    return num;
}

template<typename Number>
internal Number
clamp(Number value, Number min, Number max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
}

template<typename Float> inline Float
Modulo(Float dividend, Float divisor)
{
    Float result = fmod(dividend, divisor);
    return result;
}


// Todo(Leo): Add namespace
inline real32
Sine (real32 value)
{
    real32 result = sinf(value);
    return result;
}

inline real32
Cosine(real32 value)
{
    real32 result = cosf(value);
    return result;
}

inline real32
Tan(real32 value)
{
    real32 result = tanf(value);
    return result;
}

inline real32
ArcSine(real32 value)
{
    real32 result = asinf(value);
    return result;
}

inline real32
ArcCosine(real32 value)
{
    real32 result = acosf(value);
    return result;
}

inline real32
ArcTan2(real32 cosValue, real32 sinValue)
{
    real32 result = atan2f(cosValue, sinValue);
    return result;
}

template<typename Number>
inline Number
Sign(Number value)
{
    if (value < 0) return -1;
    if (value > 0) return 1;
    return 0;
}

internal constexpr real32 pi = 3.141592653589793f;

internal constexpr real32 degToRad = pi / 180.f;
internal constexpr real32 radToDeg = 180.0f / pi;

// Todo(Leo): remove or convert to functions
internal constexpr real32 DegToRad = pi / 180.f;
internal constexpr real32 RadToDeg = 180.0f / pi;

template<typename Number>
Number ToDegrees(Number radians)
{
    Number result = 180.0f * radians / pi;
    return result; 
}

template<typename Number>
Number ToRadians(Number degrees)
{
    Number result = pi * degrees / 180.0f;
    return result;
}

template<typename Number>
Number interpolate(Number from, Number to, Number time)
{
    Number result = (1 - time) * from + time * to;
    return result;
}

