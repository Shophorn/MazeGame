/*
Leo Tamminen
*/

struct SkySettings
{
	f32 sunHeightAngle 		= 0;
	f32 sunOrbitAngle 		= 0;
	f32 skyColourSelection 	= 0;
	f32 hdrExposure 		= 1;
	f32 hdrContrast 		= 0;
	f32 horizonHaloFalloff 	= 0;
	f32 sunHaloFalloff 		= 0;
	f32 sunDiscSize 		= 0;
	f32 sunDiscFalloff 		= 0;

	v3 skyGradientBottom 	= {};
	v3 skyGradientTop 		= {};
	v3 skyGradientGround 	= {};
	v3 horizonHaloColour 	= {};
	v3 sunHaloColour 		= {};
	v3 sunDiscColour 		= {};
	
	static constexpr auto serializedProperties = make_property_list
	(
		serialize_property("sunHeightAngle", 		&SkySettings::sunHeightAngle),
		serialize_property("sunOrbitAngle", 		&SkySettings::sunOrbitAngle),
		serialize_property("skyColourSelection", 	&SkySettings::skyColourSelection),
		serialize_property("hdrExposure", 			&SkySettings::hdrExposure),
		serialize_property("hdrContrast", 			&SkySettings::hdrContrast),
		serialize_property("horizonHaloFalloff", 	&SkySettings::horizonHaloFalloff),
		serialize_property("sunHaloFalloff", 		&SkySettings::sunHaloFalloff),
		serialize_property("sunDiscSize", 			&SkySettings::sunDiscSize),
		serialize_property("sunDiscFalloff", 		&SkySettings::sunDiscFalloff),
		serialize_property("skyGradientBottom",		&SkySettings::skyGradientBottom),
		serialize_property("skyGradientGround",		&SkySettings::skyGradientGround),
		serialize_property("skyGradientTop", 		&SkySettings::skyGradientTop),
		serialize_property("horizonHaloColour", 	&SkySettings::horizonHaloColour),
		serialize_property("sunHaloColour", 		&SkySettings::sunHaloColour),
		serialize_property("sunDiscColour", 		&SkySettings::sunDiscColour)
	);
};


internal void sky_gui(SkySettings & settings)
{
	gui_float_slider("Sky Color", &settings.skyColourSelection, 0,1);

	gui_float_slider("Sun Height Angle", &settings.sunHeightAngle, -1, 1);
	gui_float_slider("Sun Orbit Angle", &settings.sunOrbitAngle, 0, 1);

	gui_line();

	gui_colour_rgb("Sky Gradient Ground", &settings.skyGradientGround, GUI_COLOR_FLAGS_HDR);
	gui_colour_rgb("Sky Gradient Bottom", &settings.skyGradientBottom, GUI_COLOR_FLAGS_HDR);
	gui_colour_rgb("Sky Gradient Top", &settings.skyGradientTop, GUI_COLOR_FLAGS_HDR);

	gui_line();

	gui_colour_rgb("Horizon Halo Color", &settings.horizonHaloColour, GUI_COLOR_FLAGS_HDR);
	gui_float_slider("Horizon Halo Falloff", &settings.horizonHaloFalloff, 0.0001, 1);

	gui_line();

	gui_colour_rgb("Sun Halo Color", &settings.sunHaloColour, GUI_COLOR_FLAGS_HDR);
	gui_float_slider("Sun Halo Falloff", &settings.sunHaloFalloff, 0.0001, 1);

	gui_line();

	gui_colour_rgb("Sun Disc Color", &settings.sunDiscColour, GUI_COLOR_FLAGS_HDR);
	gui_float_slider("Sun Disc Size", &settings.sunDiscSize, 0, 0.01);
	gui_float_slider("Sun Disc Falloff", &settings.sunDiscFalloff, 0, 1);

	gui_line();

	gui_float_slider("HDR Exposure", &settings.hdrExposure, 0.1, 10);
	gui_float_slider("HDR Contrast", &settings.hdrContrast, -1, 1);
};
