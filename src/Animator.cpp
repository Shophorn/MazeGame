/*=============================================================================
Leo Tamminen

Animation System
=============================================================================*/

#include "animations.hpp"

struct AnimatedBone
{
	// Properties
	Transform3D boneSpaceDefaultTransform;
	m44 		inverseBindMatrix;
	s32 		parent;
};

internal AnimatedBone
make_bone (Transform3D boneSpaceDefaultTransform, m44 inverseBindMatrix, s32 parent)
{
	AnimatedBone bone = {boneSpaceDefaultTransform, inverseBindMatrix, parent};
	return bone;
}

struct AnimatedSkeleton
{
	s32 			boneCount;
	AnimatedBone * 	bones;
};

struct SkeletonAnimator
{
	AnimatedSkeleton *	skeleton;
	Array2<Transform3D> boneBoneSpaceTransforms;

	Animation const ** 	animations;
	f32 *				weights;
	s32					animationCount;
	f32 				animationTime;
};


s32 previous_keyframe(s32 keyframeCount, f32 * keyframeTimes, f32 time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframe's time, so that it was actually the last. */

	// Todo(Leo): Reduce branching maybe.

	// Note(Leo): Last keyframe from previous loop round
	if (time < keyframeTimes[0])
	{
		return keyframeCount - 1;
	}

	for (s32 i = 0; i < keyframeCount - 1; ++i)
	{
		f32 previousTime = keyframeTimes[i];
		f32 nextTime = keyframeTimes[i + 1];

		if (previousTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return keyframeCount - 1;
}

internal f32 get_relative_time(	f32 previousKeyframeTime,
								f32 nextKeyframeTime,
								f32 animationTime,
								f32 animationDuration)
{
	f32 timeToNow = previousKeyframeTime < animationTime ?
					animationTime - previousKeyframeTime :
					animationDuration - animationTime + previousKeyframeTime;

	f32 timeToNext = 	previousKeyframeTime < nextKeyframeTime ?
						nextKeyframeTime - previousKeyframeTime :
						animationDuration - nextKeyframeTime + previousKeyframeTime;

	f32 relativeTime = timeToNow / timeToNext;
	return relativeTime;
}

internal v3 animation_get_translation(Animation const & animation, s32 channelIndex, f32 time)
{
	AnimationChannelInfo const & info = animation.translationChannelInfos[channelIndex];

	time = mod_f32(time, animation.duration);

	f32 * keyframeTimes 		= animation.translationTimes + info.firstIndex;
	v3 * keyframeValues 		= animation.translations + info.firstIndex;

	s32 previousKeyframeIndex 	= previous_keyframe(info.keyframeCount, keyframeTimes, time);

	v3 result = {};

	if (info.interpolationMode == INTERPOLATION_MODE_STEP)
	{
		result = keyframeValues[previousKeyframeIndex];
	}
	else if (info.interpolationMode == INTERPOLATION_MODE_LINEAR)
	{
		s32 nextKeyframeIndex = (previousKeyframeIndex + 1) % info.keyframeCount;

		f32 previousTime 	= keyframeTimes[previousKeyframeIndex];
		f32 nextTime 		= keyframeTimes[nextKeyframeIndex];
		f32 relativeTime 	= get_relative_time(previousTime, nextTime, time, animation.duration);

		v3 previousValue 	= keyframeValues[previousKeyframeIndex];
		v3 nextValue 		= keyframeValues[nextKeyframeIndex];

		result 				= v3_lerp(previousValue, nextValue, relativeTime);
	}
	else
	{
		log_debug(FILE_ADDRESS, "Bad animation interpolation mode");
		result = keyframeValues[previousKeyframeIndex];
	}

	return result;
}


internal quaternion animation_get_rotation(Animation const & animation, s32 channelIndex, f32 time)
{
	AnimationChannelInfo const & info = animation.rotationChannelInfos[channelIndex];

	time = mod_f32(time, animation.duration);

	f32 * keyframeTimes 		= animation.rotationTimes + info.firstIndex;
	quaternion * keyframeValues = animation.rotations + info.firstIndex;
	s32 previousKeyframeIndex 	= previous_keyframe(info.keyframeCount, keyframeTimes, time);

	quaternion result = {};

	if (info.interpolationMode == INTERPOLATION_MODE_STEP)
	{
		result = keyframeValues[previousKeyframeIndex];
	}
	else if (info.interpolationMode == INTERPOLATION_MODE_LINEAR)
	{
		s32 nextKeyframeIndex = (previousKeyframeIndex + 1) % info.keyframeCount;

		f32 previousTime 	= keyframeTimes[previousKeyframeIndex];
		f32 nextTime 		= keyframeTimes[nextKeyframeIndex];	
		f32 relativeTime 	= get_relative_time(previousTime, nextTime, time, animation.duration);

		quaternion previousValue 	= keyframeValues[previousKeyframeIndex];
		quaternion nextValue 		= keyframeValues[nextKeyframeIndex];

		result = quaternion_slerp(previousValue, nextValue, relativeTime);
	}
	else
	{
		log_debug(FILE_ADDRESS, "Sad animation interpolation mode: ", (s32)info.interpolationMode, ", channel: ", channelIndex);
		result = keyframeValues[previousKeyframeIndex];
	}

	return result;
}


void update_skeleton_animator(SkeletonAnimator & animator, f32 elapsedTime)
{
	AnimatedSkeleton & skeleton 	= *animator.skeleton;
	Animation const ** animations 	= animator.animations;
	f32 * weights 					= animator.weights;
	s32 animationCount 				= animator.animationCount;

	// ------------------------------------------------------------------------

	// Note(Leo): random amount to prevent us getting somewhere where floating point accuracy becomes problem
	constexpr f32 resetIntervalSeconds 	= 60; 
	animator.animationTime 				= mod_f32(animator.animationTime + elapsedTime, resetIntervalSeconds);

	for (s32 b = 0; b < skeleton.boneCount; ++b)
	{
		AnimatedBone const & bone = animator.skeleton->bones[b];

		v3 totalTranslation 	= {};
		f32 totalAppliedWeight 	= 0;

		quaternion totalRotation = identity_quaternion;
		f32 rotationTotalAppliedWeight = 0;

		for (s32 a = 0; a < animationCount; ++a)
		{

			f32 weight = animator.weights[a];
			if (animator.weights[a] < common_almost_zero)
			{
				continue;
			}

			Animation const & animation = *animations[a];

			/// ------------ TRANSLATION ----------------
			AnimationChannelInfo const & tChannel = animation.translationChannelInfos[b];
			if (tChannel.keyframeCount > 0)
			{
				// Todo(Leo): this is wrong, we should be able to add weight of empty animations, as 
				// it semantically animations themselves that are weighted, and not channels whatsoever
				totalAppliedWeight += weight;
				totalTranslation += weight * animation_get_translation(animation, b, animator.animationTime);
			}
			
			/// ---------- ROTATION --------------
			AnimationChannelInfo const & rChannel = animation.rotationChannelInfos[b];
			if (rChannel.keyframeCount > 0)
			{
				rotationTotalAppliedWeight 	+= weight;
				totalRotation = quaternion_slerp(	totalRotation,
													animation_get_rotation(animation, b, animator.animationTime),
													weight / rotationTotalAppliedWeight);
			}
		}

		f32 defaultPoseWeight = 1 - totalAppliedWeight;
		totalTranslation += bone.boneSpaceDefaultTransform.position * defaultPoseWeight;

		animator.boneBoneSpaceTransforms[b].position = totalTranslation;


		f32 rotationDefaultPoseWeight 	= 1 - rotationTotalAppliedWeight;
		totalRotation 			= quaternion_slerp(totalRotation, bone.boneSpaceDefaultTransform.rotation, rotationDefaultPoseWeight);

		animator.boneBoneSpaceTransforms[b].rotation = totalRotation;
	}
}

// Here lies some proof for way we add animations, particularly rotations, together
// a, b, c, d
// x, y, z, w

// --> t = a * x / (x + y + z + w)
// 		+ b * y / (x + y + z + w)
// 		+ c * z / (x + y + z + w)
// 		+ d * w / (x + y + z + w)

// x + y + z + w = 1

// --> t = a * x
// 		+ b * y
// 		+ c * z 
// 		+ d * w

// t = 0

// x = 0.1
// y = 0.4
// z = 0.3
// y = 0.2

// t = lerp(t, a, 0.1/0.1)		-> a: 1
// t = lerp(t, b, 0.4/0.5)		-> a: 0.2 b: 0.8
// t = lerp(t, c, 0.3/0.8)		-> a: 0.125 b: 0.5 c: 0.375
// t = lerp(t, d, 0.2/1.0)		-> a: 0.1 b: 0.4 c: 0.3 d: 0.2

// t = 0

// t = lerp(t, a, x / x)				-> a0: x / x
// t = lerp(t, b, y / (x + y)) 		-> a1: a0 * x / (x + y)
// t = lerp(t, c, z / (x + y + z))		-> a2: a1 * (x + y) / (x + y + z)
// t = lerp(t, d, w / (x + y + z + w))	-> a3: a2 * (x + y + z) / (x + y + z + w)

// a = (((x / x) * x / (x + y)) * (x + y) / (x + y + z)) * (x + y + z) / (x + y + z + w)

// // n = previous_round_value * previous_round_total_weight / current_total_weight

// t = lerp(t, b, y / (x + y)) 		-> b0: y / (x + y)
// t = lerp(t, c, z / (x + y + z))		-> b1: b0 * (x + y) / (x + y + z)
// t = lerp(t, d, w / (x + y + z + w))	-> b2: b1 * (x + y + z) / (x + y + z + w)

// b = ((y / (x + y)) * (x + y) / (x + y + z)) * (x + y + z) / (x + y + z + w)



// t = lerp(t, c, z / (x + y + z))		-> c0: z / (x + y + z)
// t = lerp(t, d, w / (x + y + z + w))	-> c1: c0 * (x + y + z) / (x + y + w + z)

// c = z / (x + y + z) * (x + y + z) / (x + y + w + z)

// t = lerp(t, d, w /(x + y + z + w)) 	-> d0: w / (x + y + z + w) 

// d = w / (x + y + z + w)

// ------------

// 	a = (((x / x) * x / (x + y)) * (x + y) / (x + y + z)) * (x + y + z) / (x + y + z + w)
// 	b = ((y / (x + y)) * (x + y) / (x + y + z)) * (x + y + z) / (x + y + z + w)
// 	c = z / (x + y + z) * (x + y + z) / (x + y + w + z)
// 	d = w / (x + y + z + w)

// (x + y + z + w) = 1

// 	// Canceling pairs and substituting (x + y + z + w) = 1 leaves us back to correct values :)
// 	a = x / /*x * x*/ / /*(x + y) * (x + y)*/ / /*(x + y + z) * (x + y + z)*/ / 1
// 	b = y / /*(x + y) * (x + y)*/ / /*(x + y + z) * (x + y + z)*/ / 1
// 	c = z / /*(x + y + z) * (x + y + z)*/ / 1
// 	d = w / 1
// 	// mot



// t = 0
// t = lerp(t, a, 1)			-> a: 1
// t = lerp(t, b, 0.5)			-> a: 0.5, b: 0.5
// t = lerp(t, c, 0.33)		-> a: 0.33, b: 0.33, c: 0.33
// t = lerp(t, d, 0.25)		-> a: 0.25, b: 0.25, c: 0.25, d: 0.25