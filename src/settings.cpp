/*
Leo Tamminen

Persistent setting serialization stuff.

Add needed settings with SET_SETTING macro to SETTINGS_LIST.
Type enum is only used in parsing and formatting, other call sites
should use value_XX fields in SerializedSetting union
*/

struct Settings
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
	v3 horizonHaloColour 	= {};
	v3 sunHaloColour 		= {};
	v3 sunDiscColour 		= {};
	
	static constexpr auto serializedProperties = make_property_list
	(
		serialized_property("sunHeightAngle", 		&Settings::sunHeightAngle),
		serialized_property("sunOrbitAngle", 		&Settings::sunOrbitAngle),
		serialized_property("skyColourSelection", 	&Settings::skyColourSelection),
		serialized_property("hdrExposure", 			&Settings::hdrExposure),
		serialized_property("hdrContrast", 			&Settings::hdrContrast),
		serialized_property("horizonHaloFalloff", 	&Settings::horizonHaloFalloff),
		serialized_property("sunHaloFalloff", 		&Settings::sunHaloFalloff),
		serialized_property("sunDiscSize", 			&Settings::sunDiscSize),
		serialized_property("sunDiscFalloff", 		&Settings::sunDiscFalloff),
		serialized_property("skyGradientBottom",	&Settings::skyGradientBottom),
		serialized_property("skyGradientTop", 		&Settings::skyGradientTop),
		serialized_property("horizonHaloColour", 	&Settings::horizonHaloColour),
		serialized_property("sunHaloColour", 		&Settings::sunHaloColour),
		serialized_property("sunDiscColour", 		&Settings::sunDiscColour)
	);
};
