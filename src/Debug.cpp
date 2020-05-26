// Note(Leo): Using this we don't need to bother to make new rotation each time
// enum : s32 { ORIENT_2D_XY, ORIENT_2D_XZ, ORIENT_2D_YZ };


enum : s32 
{
	DEBUG_OFF,
	DEBUG_PLAYER,
	DEBUG_NPC,
	DEBUG_BACKGROUND,

	DEBUG_LEVEL_COUNT
};

s32 global_DebugDrawLevel = DEBUG_OFF;

void debug_draw_axes(m44 transform, float radius, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 rayStart 	= multiply_point(transform, {0,0,0});

	v3 rayRight 	= rayStart + multiply_direction(transform, {radius,0,0});
	v3 rayForward 	= rayStart + multiply_direction(transform, {0,radius,0});
	v3 rayUp 		= rayStart + multiply_direction(transform, {0,0,radius});

	v3 points [2] = {rayStart};

	points[1] = rayRight;
	platformApi->draw_lines(platformGraphics, 2, points, {1, 0, 0, 1});

	points[1] = rayForward;
	platformApi->draw_lines(platformGraphics, 2, points, {0, 1, 0, 1});

	points[1] = rayUp;
	platformApi->draw_lines(platformGraphics, 2, points, {0, 0, 1, 1});			
};

void debug_draw_diamond(m44 transform, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 xNeg = multiply_point(transform, v3{-1, 0, 0});
	v3 xPos = multiply_point(transform, v3{1, 0, 0});
		
	v3 yNeg = multiply_point(transform, v3{0, -1, 0});
	v3 yPos = multiply_point(transform, v3{0, 1, 0});

	v3 zNeg = multiply_point(transform, v3{0, 0, -1});
	v3 zPos = multiply_point(transform, v3{0, 0, 1});

	v3 points [] =
	{
		// xy plane
		xNeg, yNeg,
		yNeg, xPos,
		xPos, yPos,
		yPos, xNeg,

		// xz plane
		xNeg, zNeg,
		zNeg, xPos,
		xPos, zPos,
		zPos, xNeg,

		// yz plane
		yNeg, zNeg,
		zNeg, yPos,
		yPos, zPos,
		zPos, yNeg,
	};

	platformApi->draw_lines(platformGraphics, 24, points, color);
}

void debug_draw_circle_xy(v3 position, f32 radius, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 points[32];
	s32 pointCount = array_count(points);
	s32 lineCount = pointCount / 2;

	f32 angle = tau / lineCount;

	for (s32 line = 0; line < lineCount; ++line)
	{
		s32 point = line * 2;

		points[point] = v3{cosine(line * angle) * radius, sine(line * angle) * radius} + position;

		s32 next = (line + 1) % lineCount;
		points[point + 1] = v3{cosine(next * angle) * radius, sine(next * angle) * radius} + position;
	}
	platformApi->draw_lines(platformGraphics, 32, points, color);

}

// Note(Leo): this was wrong but it yielded a nice pattern so I saved it :)
// void debug_draw_circle_xy(v3 position, f32 radius, v4 color, s32 level)
// {
// 	Line lines [16] = {};
// 	f32 angle = tau / 16;

// 	for (s32 i = 0; i < 16; ++i)
// 	{
// 		// f32 s = sine(i * angle);
// 		// f32 c = cosine(i * angle);

// 		lines[i].start 	= v3{cosine(i * angle) * radius, sine(i * angle) * radius} + position;

		// Note(Leo): error was here, should have been: (i + 1) % 16....
// 		s32 next = (i + i) / 16;
// 		lines[i].end 	= v3{cosine(next * angle) * radius, sine(next * angle) * radius} + position;
// 	}

// 	debug_draw_lines(lines, 16, color);
// }

void debug_draw_diamond_xy(m44 transform, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 corners [4] =
	{
		multiply_point(transform, v3{1,0,0}),
		multiply_point(transform, v3{0,1,0}),
		multiply_point(transform, v3{-1,0,0}),
		multiply_point(transform, v3{0,-1,0}),
	};

	v3 points [] = 
	{
		corners[0], corners[1],
		corners[1], corners[2],
		corners[2], corners[3],
		corners[3], corners[0],
	};

	platformApi->draw_lines(platformGraphics, 8, points, color);
}

void debug_draw_diamond_xz(m44 transform, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 corners [4] =
	{
		multiply_point(transform, v3{1,0,0}),
		multiply_point(transform, v3{0,0,1}),
		multiply_point(transform, v3{-1,0,0}),
		multiply_point(transform, v3{0,0,-1}),
	};

	v3 points [] = 
	{
		corners[0], corners[1],
		corners[1], corners[2],
		corners[2], corners[3],
		corners[3], corners[0],
	};

	platformApi->draw_lines(platformGraphics, 8, points, color);
}

void debug_draw_diamond_yz(m44 transform, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 corners [4] =
	{
		multiply_point(transform, v3{0,1,0}),
		multiply_point(transform, v3{0,0,1}),
		multiply_point(transform, v3{0,-1,0}),
		multiply_point(transform, v3{0,0,-1}),
	};

	v3 points [] = 
	{
		corners[0], corners[1],
		corners[1], corners[2],
		corners[2], corners[3],
		corners[3], corners[0],
	};

	platformApi->draw_lines(platformGraphics, 8, points, color);
}

void debug_draw_lines(s32 count, v3 * points, v4 color, s32 level)//v3 start, v3 end, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	platformApi->draw_lines(platformGraphics, count, points, color);
}

void debug_draw_vector(v3 position, v3 vector, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	f32 size 		= 0.1f; 
	f32 length 		= magnitude_v3(vector);
	size 			= math::min(size, length / 2);

	v3 binormal 	= cross_v3(vector, up_v3);
	v3 up 			= normalize_v3(cross_v3(vector, binormal));

	v3 direction 	= normalize_v3(vector);
	v3 cornerA 		= position + direction * (length - size) + up * size;
	v3 cornerB 		= position + direction * (length - size) - up * size;

	v3 points[] =
	{
		position, position + vector,
		position + vector, cornerA,
		position + vector, cornerB,
		cornerA, cornerB
	};

	platformApi->draw_lines(platformGraphics, 8, points, color);

}

void debug_draw_box(m44 transform, v4 color, s32 level)
{
	if (level > global_DebugDrawLevel)
		return;

	v3 corners [] =
	{
		multiply_point(transform, {-1,-1,-1}),
		multiply_point(transform, { 1,-1,-1}),
		multiply_point(transform, {-1, 1,-1}),
		multiply_point(transform, { 1, 1,-1}),

		multiply_point(transform, {-1,-1, 1}),
		multiply_point(transform, { 1,-1, 1}),
		multiply_point(transform, {-1, 1, 1}),
		multiply_point(transform, { 1, 1, 1}),
	};

	v3 lines []
	{
		corners[0], corners[1],
		corners[2], corners[3],
		corners[4], corners[5],
		corners[6], corners[7],
	
		corners[0], corners[2],
		corners[1], corners[3],
		corners[4], corners[6],
		corners[5], corners[7],
	
		corners[0], corners[4],
		corners[1], corners[5],
		corners[2], corners[6],
		corners[3], corners[7],
	};

	platformApi->draw_lines(platformGraphics, 24, lines, color);
}
