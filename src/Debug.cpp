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

	void draw_diamond(	platform::Graphics * graphics,
							m44 transform,
							v4 color,
							platform::Functions * functions)
	{
		v3 xNeg = multiply_point(transform, v3{-1, 0, 0});
		v3 xPos = multiply_point(transform, v3{1, 0, 0});
			
		v3 yNeg = multiply_point(transform, v3{0, -1, 0});
		v3 yPos = multiply_point(transform, v3{0, 1, 0});

		v3 zNeg = multiply_point(transform, v3{0, 0, -1});
		v3 zPos = multiply_point(transform, v3{0, 0, 1});

		// xy plane
		functions->draw_line(graphics, xNeg, yNeg, 1.0f, color);
		functions->draw_line(graphics, yNeg, xPos, 1.0f, color);
		functions->draw_line(graphics, xPos, yPos, 1.0f, color);
		functions->draw_line(graphics, yPos, xNeg, 1.0f, color);

		// xz plane
		functions->draw_line(graphics, xNeg, zNeg, 1.0f, color);
		functions->draw_line(graphics, zNeg, xPos, 1.0f, color);
		functions->draw_line(graphics, xPos, zPos, 1.0f, color);
		functions->draw_line(graphics, zPos, xNeg, 1.0f, color);

		// yz plane
		functions->draw_line(graphics, yNeg, zNeg, 1.0f, color);
		functions->draw_line(graphics, zNeg, yPos, 1.0f, color);
		functions->draw_line(graphics, yPos, zPos, 1.0f, color);
		functions->draw_line(graphics, zPos, yNeg, 1.0f, color);
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