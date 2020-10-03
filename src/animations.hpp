
enum InterpolationMode
{
	INTERPOLATION_MODE_STEP,
	INTERPOLATION_MODE_LINEAR,
	INTERPOLATION_MODE_CUBICSPLINE,
};

struct AnimationChannelInfo
{
	s32 				keyframeCount;
	s32 				firstIndex;
	InterpolationMode 	interpolationMode;
};

struct Animation
{
	s32 channelCount;
	f32 duration;

	AnimationChannelInfo * 	translationChannelInfos;		
	AnimationChannelInfo * 	rotationChannelInfos;

	f32 * 					translationTimes;
	f32 *					rotationTimes;

	v3 * 					translations;
	quaternion * 			rotations;
};

