/*
Leo Tamminen

Colour constants and helper functions
*/
internal u32 colour_rgba_u32(v4 color)
{
	u32 pixel = ((u32)(color.r * 255))
				| (((u32)(color.g * 255)) << 8)
				| (((u32)(color.b * 255)) << 16)
				| (((u32)(color.a * 255)) << 24);
	return pixel;
}

internal constexpr v4 colour_v4_from_rgb_255(u8 r, u8 g, u8 b)
{
	v4 colour = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
	return colour;
}

internal constexpr v4 colour_rgb_alpha(v3 rgb, f32 alpha)
{
	v4 colour = {rgb.x, rgb.y, rgb.z, alpha};
	return colour;
}

internal u32 colour_rgb_alpha_32(v3 colour, f32 alpha)
{
	return colour_rgba_u32(colour_rgb_alpha(colour, alpha));
}

internal v4 colour_multiply(v4 a, v4 b)
{
	a = {	a.r * b.r,
			a.g * b.g,
			a.b * b.b,
			a.a * b.a};
	return a;
}

constexpr v4 colour_white 			= {1,1,1,1};
constexpr v4 colour_gray			= {0.5, 0.5, 0.5, 1};
constexpr v4 colour_black 			= {0,0,0,1};

constexpr v4 colour_aqua_blue 		= colour_v4_from_rgb_255(51, 255, 255);
constexpr v4 colour_raw_umber 		= colour_v4_from_rgb_255(130, 102, 68);

constexpr v4 colour_bright_red 		= {1, 0, 0, 1};
constexpr v4 colour_bright_blue 	= {0, 0, 1, 1};
constexpr v4 colour_bright_green 	= {0, 1, 0, 1};
constexpr v4 colour_bright_yellow 	= {1, 1, 0, 1};
constexpr v4 colour_bright_purple 	= {1, 0, 1, 1};
constexpr v4 colour_bright_cyan 	= {0, 1, 1, 1};

constexpr v4 colour_dark_green 		= {0, 0.6, 0, 1};
constexpr v4 colour_dark_red 		= {0.6, 0, 0, 1};

constexpr v4 colour_bump 			= {0.5, 0.5, 1.0, 0.0};

constexpr v4 colour_muted_red 		= {0.8, 0.2, 0.3, 1.0};
constexpr v4 colour_muted_green 	= {0.2, 0.8, 0.3, 1.0};
constexpr v4 colour_muted_blue 		= {0.2, 0.3, 0.8, 1.0};
constexpr v4 colour_muted_yellow 	= {0.8, 0.8, 0.2, 1.0};
constexpr v4 color_muted_purple 	= {0.8, 0.2, 0.8, 1};
