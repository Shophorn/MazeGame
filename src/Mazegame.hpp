struct World
{
	/*
	Note(Leo): Right handed coordinate system, x right, y, forward, z up.
	*/

	static constexpr vector3 Left 		= {-1,  0,  0};
	static constexpr vector3 Right 		= { 1,  0,  0};
	static constexpr vector3 Back 		= { 0, -1,  0};
	static constexpr vector3 Forward 	= { 0,  1,  0};
	static constexpr vector3 Down 		= { 0,  0, -1};
	static constexpr vector3 Up 		= { 0,  0,  1};
};