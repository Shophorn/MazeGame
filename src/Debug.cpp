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

	void draw_diamond(	platform::Graphics * graphics,
						m44 transform,
						float radius,
						platform::Functions * functions)
	{
		v3 points [4] =
		{
			multiply_point(transform, v3{0, 1, 0} * radius),
			multiply_point(transform, v3{1, 0, 0} * radius),
			multiply_point(transform, v3{0, -1, 0} * radius),
			multiply_point(transform, v3{-1, 0, 0} * radius)
		};

		functions->draw_line(graphics, points[0], points[1], 1.0f, {1,1,1,1});
		functions->draw_line(graphics, points[1], points[2], 1.0f, {1,1,1,1});
		functions->draw_line(graphics, points[2], points[3], 1.0f, {1,1,1,1});
		functions->draw_line(graphics, points[3], points[0], 1.0f, {1,1,1,1});
	}
}