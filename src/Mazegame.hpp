struct World
{
	/*
	Note(Leo): Right handed coordinate system, x right, y, forward, z up.
	*/

	static constexpr Vector3 Left 		= {-1,  0,  0};
	static constexpr Vector3 Right 		= { 1,  0,  0};
	static constexpr Vector3 Back 		= { 0, -1,  0};
	static constexpr Vector3 Forward 	= { 0,  1,  0};
	static constexpr Vector3 Down 		= { 0,  0, -1};
	static constexpr Vector3 Up 		= { 0,  0,  1};
};