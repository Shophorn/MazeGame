namespace debug
{
	void draw_axes(platform::Graphics * graphics, m44 transform, platform::Functions * functions)
	{
		v3 rayStart 	= multiply_point(transform, {0,0,0});
	
		v3 rayRight 	= rayStart + multiply_direction(transform, {0.5f,0,0});
		v3 rayForward 	= rayStart + multiply_direction(transform, {0,0.5f,0});
		v3 rayUp 		= rayStart + multiply_direction(transform, {0,0,0.5f});

		functions->draw_line(graphics, rayStart, rayRight, 25.0f, {1, 0, 0, 1});
		functions->draw_line(graphics, rayStart, rayForward, 25.0f, {0, 1, 0, 1});
		functions->draw_line(graphics, rayStart, rayUp, 25.0f, {0, 0, 1, 1});			
	};
}