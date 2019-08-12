// TODO(Leo): Proper unit testing!!
// Note(Leo): For now. Actually implement these ourself, for the kicks of course.
#include <cmath>

internal real32
Root2(real32 value)
{
    real32 result = sqrt(value);
    return result; 
}

internal int32
FloorToInt32(real32 value)
{
    int32 result = static_cast<int32>(value);
    if (value < 0)
        result -= 1;
    return result;
}

template<typename IntegerType, typename FloatType>
IntegerType
FloorTo(FloatType value)
{
    int64 floorValue = static_cast<int64>(value);
    if (value < 0)
        floorValue = floorValue -1;
    return static_cast<IntegerType>(floorValue);
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

internal int32
RoundToInt32(real32 value)
{
    if (value < 0.0f)
        value -= 0.5f;
    else
        value += 0.5f;
    return static_cast<int32>(value);
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
Clamp(Number value, Number min, Number max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
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

internal constexpr real32 pi = 3.141592653589793f;

internal constexpr real32 DegToRad = pi / 180.f;
internal constexpr real32 RadToDeg = 180.0f / pi;