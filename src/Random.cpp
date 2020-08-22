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

internal f32 random_value()
{
	f32 value 	= static_cast<f32>(xor128());
	f32 max 	= static_cast<f32>(max_value_u32);
	f32 result 	= value / max;

	return result;	
}

internal bool random_choice()
{
	return xor128() & 0x00000001;	
}

internal bool32 random_choice_probability(f32 probability)
{
	bool32 result = random_value() < probability;
	return result;
}

internal f32 random_range(f32 min, f32 max)
{
	AssertMsg (min <= max, "'min' must be smaller than 'max'");

	f32 value = static_cast<f32>(xor128()) / static_cast<f32>(max_value_u32);
	f32 range = max - min;
	f32 result = min + (value * range);

	return result;    
}

internal v3 random_inside_unit_square()
{
	v3 result = {random_range(0,1), random_range(0,1), 0};
	return result;
}

// internal v3 random_inside_circle()
// {

// }
