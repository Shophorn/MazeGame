/*=============================================================================
Leo Tamminen

Pseudo random generator. This is rather important for the game since levels
are procedurally generated, so we will implement this ourselves.
=============================================================================*/

// Note(Leo): From https://codingforspeed.com/using-faster-psudo-random-generator-xorshift/
u32 xor128()
{
  	static u32 x = 123456789;
  	static u32 y = 362436069;
  	static u32 z = 521288629;
  	static u32 w = 88675123;
  	u32 t;
 	t = x ^ (x << 11);   
  	x = y; y = z; z = w;   
  	return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}

internal f32
RandomValue()
{
  	f32 value 	= static_cast<f32>(xor128());
  	f32 max 		= static_cast<f32>(math::highest_value<u32>);
  	f32 result 	= value / max;

  	return result;
}

internal f32
RandomRange(f32 min, f32 max)
{
    DEBUG_ASSERT (min <= max, "'min' must be smaller than 'max'");

    f32 value = static_cast<f32>(xor128()) / static_cast<f32>(math::highest_value<u32>);
    f32 range = max - min;
    f32 result = min + (value * range);

    return result;    
}