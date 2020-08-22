/*
Leo Tamminen

Persistent setting serialization stuff.

Add needed settings with SET_SETTING macro to SETTINGS_LIST.
Type enum is only used in parsing and formatting, other call sites
should use value_XX fields in SerializedSetting union
*/

enum SerializedSettingType : s32
{
	TYPE_F32,
};

#define SETTINGS_LIST \
		SET_SETTING(sunHeightAngle, 	TYPE_F32, 0) \
		SET_SETTING(sunOrbitAngle, 		TYPE_F32, 0) \
		SET_SETTING(skyColourSelection, TYPE_F32, 0) \
		SET_SETTING(hdrExposure, 		TYPE_F32, 1) \
		SET_SETTING(hdrContrast, 		TYPE_F32, 0)

struct SerializedSetting
{
	char const * 			id;
	SerializedSettingType 	type;

	union
	{
		f32 value_f32;
	};
};

struct SerializedSettings
{
	#define SET_SETTING(name, type, value) SerializedSetting name;
	struct { SETTINGS_LIST };
	#undef SET_SETTING

	bool32 dirty;

	#define SET_SETTING(name, type, value) + 1
	static constexpr s32 count = SETTINGS_LIST;
	#undef SET_SETTING

	SerializedSetting & operator [] (s32 index)
	{
		Assert(index < count);

		SerializedSetting * settingsArray = (SerializedSetting*)this;
		return settingsArray[index];
	}
};

static SerializedSettings init_settings()
{
	SerializedSettings settings = {};

	#define SET_SETTING(name, type, value) \
		settings.name = {#name, type}; \
		if (type == TYPE_F32) { settings.name.value_f32 = value; }

	SETTINGS_LIST;
	#undef SET_SETTING

	return settings;
}