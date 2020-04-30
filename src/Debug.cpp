// Note(Leo): Using this we don't need to bother to make new rotation each time
enum : s32 { ORIENT_2D_XY, ORIENT_2D_XZ, ORIENT_2D_YZ };

namespace debug
{
	void draw_axes(	platform::Graphics * graphics,
					m44 transform,
					float radius,
					platform::Functions * functions)
	{
		v3 rayStart 	= multiply_point(transform, {0,0,0});
	
		v3 rayRight 	= rayStart + multiply_direction(transform, {radius,0,0});
		v3 rayForward 	= rayStart + multiply_direction(transform, {0,radius,0});
		v3 rayUp 		= rayStart + multiply_direction(transform, {0,0,radius});

		functions->draw_line(graphics, rayStart, rayRight, 25.0f, {1, 0, 0, 1});
		functions->draw_line(graphics, rayStart, rayForward, 25.0f, {0, 1, 0, 1});
		functions->draw_line(graphics, rayStart, rayUp, 25.0f, {0, 0, 1, 1});			
	};

	void draw_diamond_xy(	platform::Graphics * graphics,
							m44 transform,
							v4 color,
							platform::Functions * functions)
	{
		v3 points [4] =
		{
			multiply_point(transform, v3{0, 1, 0}),
			multiply_point(transform, v3{1, 0, 0}),
			multiply_point(transform, v3{0, -1, 0}),
			multiply_point(transform, v3{-1, 0, 0})
		};

		functions->draw_line(graphics, points[0], points[1], 1.0f, color);
		functions->draw_line(graphics, points[1], points[2], 1.0f, color);
		functions->draw_line(graphics, points[2], points[3], 1.0f, color);
		functions->draw_line(graphics, points[3], points[0], 1.0f, color);
	}

	void draw_diamond(m44 transform, v4 color)
	{
		v3 xNeg = multiply_point(transform, v3{-1, 0, 0});
		v3 xPos = multiply_point(transform, v3{1, 0, 0});
			
		v3 yNeg = multiply_point(transform, v3{0, -1, 0});
		v3 yPos = multiply_point(transform, v3{0, 1, 0});

		v3 zNeg = multiply_point(transform, v3{0, 0, -1});
		v3 zPos = multiply_point(transform, v3{0, 0, 1});

		// xy plane
		platformApi->draw_line(platformGraphics, xNeg, yNeg, 1.0f, color);
		platformApi->draw_line(platformGraphics, yNeg, xPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, xPos, yPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, yPos, xNeg, 1.0f, color);

		// xz plane
		platformApi->draw_line(platformGraphics, xNeg, zNeg, 1.0f, color);
		platformApi->draw_line(platformGraphics, zNeg, xPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, xPos, zPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, zPos, xNeg, 1.0f, color);

		// yz plane
		platformApi->draw_line(platformGraphics, yNeg, zNeg, 1.0f, color);
		platformApi->draw_line(platformGraphics, zNeg, yPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, yPos, zPos, 1.0f, color);
		platformApi->draw_line(platformGraphics, zPos, yNeg, 1.0f, color);
	}

	struct Line
	{
		v3 start, end;
	};

	void draw_lines(Line * lines, s32 lineCount, v4 color)
	{
		for (int i = 0; i < lineCount; ++i)
		{
			platformApi->draw_line(platformGraphics, lines[i].start, lines[i].end, 1.0f, color);
		}
	}

	void draw_circle_xy(v3 position, f32 radius, v4 color)
	{
		Line lines [16];
		s32 lineCount = array_count(lines);

		f32 angle = tau / lineCount;

		for (s32 i = 0; i < lineCount; ++i)
		{
			lines[i].start 	= v3{cosine(i * angle) * radius, sine(i * angle) * radius} + position;

			s32 next = (i + 1) % lineCount;
			lines[i].end 	= v3{cosine(next * angle) * radius, sine(next * angle) * radius} + position;
		}

		draw_lines(lines, lineCount, color);
	}

	// Note(Leo): this was wrong but it yielded a nice pattern so I saved it :)
	// void draw_circle_xy(v3 position, f32 radius, v4 color)
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

	// 	draw_lines(lines, 16, color);
	// }

	void draw_diamond_2d(m44 transform, v4 color, s32 orientation)
	{
		v3 corners [4];

		switch (orientation)
		{
			case ORIENT_2D_XY:
				corners[0] = multiply_point(transform, v3{1,0,0});
				corners[1] = multiply_point(transform, v3{0,1,0});
				corners[2] = multiply_point(transform, v3{-1,0,0});
				corners[3] = multiply_point(transform, v3{0,-1,0});
				break;

			case ORIENT_2D_XZ:
				corners[0] = multiply_point(transform, v3{1,0,0});
				corners[1] = multiply_point(transform, v3{0,0,1});
				corners[2] = multiply_point(transform, v3{-1,0,0});
				corners[3] = multiply_point(transform, v3{0,0,-1});
				break;

			case ORIENT_2D_YZ: 
				corners[0] = multiply_point(transform, v3{0,1,0});
				corners[1] = multiply_point(transform, v3{0,0,1});
				corners[2] = multiply_point(transform, v3{0,-1,0});
				corners[3] = multiply_point(transform, v3{0,0,-1});
				break;
		}

		for (s32 i = 0; i < 4; ++i)
		{
			platformApi->draw_line(platformGraphics, corners[i], corners[(i + 1) % 4], 1.0f, color);
		}
	}




	void draw_line(v3 start, v3 end, v4 color)
	{
		platformApi->draw_line(platformGraphics, start, end, 1.0f, color);
	}

	void draw_vector(v3 position, v3 vector, v4 color, f32 size = 0.1f)
	{
		f32 length 		= vector.magnitude();
		size 			= math::min(size, length / 2);

		v3 binormal 	= vector::cross(vector, v3::up);
		v3 up 			= vector::cross(vector, binormal).normalized();

		v3 direction 	= vector.normalized();
		v3 cornerA 		= position + direction * (length - size) + up * size;
		v3 cornerB 		= position + direction * (length - size) - up * size;

		platformApi->draw_line(platformGraphics, position, position + vector, 1.0f, color);
		platformApi->draw_line(platformGraphics, position + vector, cornerA, 1.0f, color);
		platformApi->draw_line(platformGraphics, position + vector, cornerB, 1.0f, color);
		platformApi->draw_line(platformGraphics, cornerA, cornerB, 1.0f, color);

	}

}