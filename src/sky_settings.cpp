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


static f32 * v3_array(v3 & v)
{
	return reinterpret_cast<f32*>(&v);
}

internal void sky_gui(SkySettings & settings)
{
	using namespace ImGui;	
	Begin("Sky Settings");

	SliderFloat("Sun Height Angle", &settings.sunHeightAngle, -1, 1);
	SliderFloat("Sun Orbit Angle", &settings.sunOrbitAngle, -1, 1);

	Separator();

	ColorEdit3("Sky Gradient Ground", v3_array(settings.skyGradientGround), ImGuiColorEditFlags_HDR);
	ColorEdit3("Sky Gradient Bottom", v3_array(settings.skyGradientBottom), ImGuiColorEditFlags_HDR);
	ColorEdit3("Sky Gradient Top", v3_array(settings.skyGradientTop), ImGuiColorEditFlags_HDR);

	Separator();

	ColorEdit3("Horizon Halo Color", v3_array(settings.horizonHaloColour), ImGuiColorEditFlags_HDR);
	SliderFloat("Horizon Halo Falloff", &settings.horizonHaloFalloff, 0.0001, 1);

	Separator();

	ColorEdit3("Sun Halo Color", v3_array(settings.sunHaloColour), ImGuiColorEditFlags_HDR);
	SliderFloat("Sun Halo Falloff", &settings.sunHaloFalloff, 0.0001, 1);
	
	Separator();

	ColorEdit3("Sun Disc Color", v3_array(settings.sunDiscColour), ImGuiColorEditFlags_HDR);
	SliderFloat("Sun Disc Size", &settings.sunDiscSize, 0, 0.01);
	SliderFloat("Sun Disc Falloff", &settings.sunDiscFalloff, 0, 1);

	Separator();

	SliderFloat("HDR Exposure", &settings.hdrExposure, 0.1, 10);
	SliderFloat("HDR Contrast", &settings.hdrContrast, -0.5, 1);

	End();
};