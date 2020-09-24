/*=============================================================================
Leo Tamminen

Animation System
=============================================================================*/

enum InterpolationMode
{
	INTERPOLATION_MODE_STEP,
	INTERPOLATION_MODE_LINEAR,
	INTERPOLATION_MODE_CUBICSPLINE,
};

struct TranslationChannel
{
	s32 				targetIndex;
	Array<f32> 			times;
	Array<v3> 			translations;
	InterpolationMode 	interpolationMode;
};

struct RotationChannel
{
	s32 				targetIndex;
	Array<f32> 			times;
	Array<quaternion> 	rotations;
	InterpolationMode 	interpolationMode;
};

struct Animation
{
	Array<TranslationChannel> 	translationChannels;
	Array<RotationChannel> 		rotationChannels;
	f32 						duration;
};

struct AnimatedHierarchyNode
{
	Transform3D currentTransform;
	Transform3D defaultTransform;
	s32 		parent;
};

struct AnimatedHierarchy
{
	AnimatedHierarchyNode * nodes;
	s64 					nodeCount;
};

// Property/State
struct AnimationRig
{
	Transform3D * 		root;
	Array<Transform3D*> bones;
};

struct Animator
{
	// Properties
	AnimationRig rig;
	f32 playbackSpeed	= 1.0f;

	// State
	f32 				time;
	Animation const * 	animation;
};

struct AnimatedBone
{
	// State
	Transform3D boneSpaceTransform;

	// Properties
	Transform3D boneSpaceDefaultTransform;
	m44 		inverseBindMatrix;
	s32 		parent;

	// For debugging only
	// SmallString name;
};

internal AnimatedBone
make_bone (Transform3D boneSpaceDefaultTransform, m44 inverseBindMatrix, s32 parent)
{
	AnimatedBone bone = {identity_transform,
						boneSpaceDefaultTransform,
						inverseBindMatrix,
						parent};
	return bone;
}

struct AnimatedSkeleton
{
	Array<AnimatedBone> bones;

	/* Todo (Leo): we don't need array operations, just a block of memory
	AnimatedBone * 	bones;
	s64 			boneCount;
	*/


	/* Todo(Leo): using these, we can easily animate both deforming skeletons and non-deforming
	hierarchies with same functions, that take just the animated hierarchy node array or pointer. */
	// Array<AnimatedHierarchyNode> 	bones;
	// Array<m44> 						inverseBindMatrices;
	// Array<SmallString> 				names;
};

struct SkeletonAnimator
{
	AnimatedSkeleton 	skeleton;
	
	Animation const ** 	animations;
	f32 *				weights;
	s32					animationCount;
	f32 				animationTime;
};

internal AnimationRig
make_animation_rig(Transform3D * root, Array<Transform3D*> bones)
{
	AnimationRig result 
	{
		.root 							= root,
		.bones 							= std::move(bones),
	};
	return result;
}

internal Animation
copy_animation(MemoryArena & allocator, Animation const & original)
{
	s32 translationCount 	= original.translationChannels.count();
	s32 rotationCount 		= original.rotationChannels.count();

	Animation copy = 
	{
		allocate_array<TranslationChannel>(allocator, translationCount, ALLOC_FILL),
		allocate_array<RotationChannel>(allocator, rotationCount, ALLOC_FILL),
		original.duration
	};

	for (s32 i = 0; i < translationCount; ++ i)
	{
		TranslationChannel const & channel = original.translationChannels[i];

		copy.translationChannels[i] = 
		{
			channel.targetIndex,
			copy_array(allocator, channel.times),
			copy_array(allocator, channel.translations),
			channel.interpolationMode
		};
	}

	for (s32 i = 0; i <rotationCount; ++i)
	{
		RotationChannel const & channel = original.rotationChannels[i];

		copy.rotationChannels[i] =
		{ 	
			channel.targetIndex,
			copy_array(allocator, channel.times),
			copy_array(allocator, channel.rotations),
			channel.interpolationMode
		};
	}

	return copy;
}

internal void
reverse_animation_clip(Animation & animation)
{
	for (auto & channel : animation.translationChannels)
	{
		reverse_array(channel.translations);
	
		reverse_array(channel.times);
		for (f32 & time : channel.times)
		{
			time = animation.duration - time;
		}
	}

	for (auto & channel : animation.rotationChannels)
	{
		reverse_array(channel.rotations);

		reverse_array(channel.times);
		for (f32 & time : channel.times)
		{
			time = animation.duration - time;
		}
	}
}

internal Animation
make_animation (Array<TranslationChannel> translationChannels, Array<RotationChannel> rotationChannels)
{
	f32 duration = 0;
	for (auto const & channel : translationChannels)
		duration = max_f32(duration, channel.times.last());

	for (auto const & channel : rotationChannels)
		duration = max_f32(duration, channel.times.last());

	Animation animation =
	{
		std::move(translationChannels),
		std::move(rotationChannels),
		duration
	};
	return animation;
}

s32 previous_keyframe(Array<f32> const & keyframeTimes, f32 time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframe's time, so that it was actually the last. */

	// Todo(Leo): Reduce branching maybe.

	// Note(Leo): Last keyframe from previous loop round
	if (time < keyframeTimes[0])
	{
		return keyframeTimes.count() - 1;
	}

	for (s32 i = 0; i < keyframeTimes.count() - 1; ++i)
	{
		f32 previousTime = keyframeTimes[i];
		f32 nextTime = keyframeTimes[i + 1];

		if (previousTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return keyframeTimes.count() - 1;
}

internal void
update_animator_system(PlatformInput const & input, Array<Animator> & animators, f32 elapsedTime)
{
	for (auto & animator : animators)
	{
		if (animator.animation == nullptr)
			continue;

		f32 timeStep = animator.playbackSpeed * elapsedTime;
		animator.time += timeStep;

		for (auto const & channel : animator.animation->translationChannels)
		{
			Transform3D & target = *animator.rig.bones[channel.targetIndex];

			// Before first keyframe
			if (animator.time < channel.times[0])
			{
				target.position = channel.translations[0];
			}
			// After last keyframe
			else if(bool isAfterLast = channel.times.last() < animator.time)
			{
				target.position = channel.translations.last();
			}
			else
			{
				s32 previousKeyframe 		= previous_keyframe(channel.times, animator.time);
				s32 nextKeyframe 			= previousKeyframe + 1;

				f32 previousKeyFrameTime 	= channel.times[previousKeyframe];
				f32 currentKeyFrameTime 	= channel.times[nextKeyframe];
				f32 relativeTime 			= (animator.time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

				target.position = interpolate_v3(	channel.translations[previousKeyframe],
														channel.translations[nextKeyframe],
														relativeTime); 
			}
		}

		bool framesLeft = animator.time < animator.animation->duration;

		if (framesLeft == false)
		{
			animator.animation = nullptr;
		}
	}
}

internal Animator
make_animator(AnimationRig rig)
{
	Animator result = 
	{
		.rig = std::move(rig)
	};
	return result;
}

internal void
play_animation_clip(Animator * animator, Animation const * animation)
{
	animator->animation = animation;
	animator->time 		= 0.0f;
}

f32 get_relative_time(	f32 previousKeyframeTime,
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

void update_skeleton_animator(SkeletonAnimator & animator, f32 elapsedTime)
{
	AnimatedSkeleton & skeleton 	= animator.skeleton;
	Animation const ** animations 	= animator.animations;
	f32 * weights 					= animator.weights;
	s32 animationCount 				= animator.animationCount;

	// ------------------------------------------------------------------------

	// Note(Leo): random amount to prevent us getting somewhere where floating point accuracy becomes problem
	constexpr f32 resetIntervalSeconds 	= 60; 
	animator.animationTime 				= mod_f32(animator.animationTime + elapsedTime, resetIntervalSeconds);

	// ------------------------------------------------------------------------
	/* Note(Leo): We parse current animation situation beforehand. We get somewhat
	cleaner code, and fewer conditional blocks. */

	struct ChannelInfo
	{
		void const * channel;
		f32 duration;
		f32 weight;
		f32 time;
	};

	auto translationChannelInfos = allocate_nested_arrays<ChannelInfo>(	*global_transientMemory,
																		skeleton.bones.count(),
																		animationCount);

	auto rotationChannelInfos = allocate_nested_arrays<ChannelInfo>(	*global_transientMemory,
																		skeleton.bones.count(),
																		animationCount);

	for (s32 animationIndex = 0; animationIndex < animationCount; ++animationIndex)
	{
		// Note(Leo): Only add animations that have impact
		// Todo(Leo): this can be optimized away by only passing animations that pass this condition
		if (weights[animationIndex] < 0.00001f)
		{
			continue;
		}

		f32 duration 	= animations[animationIndex]->duration;
		f32 time 		= mod_f32(animator.animationTime, duration);
		f32 weight 		= weights[animationIndex];

		for (auto const & channel : animations[animationIndex]->translationChannels)
		{
			translationChannelInfos[channel.targetIndex].push({&channel, duration, weight, time});
		}

		for (auto const & channel : animations[animationIndex]->rotationChannels)
		{
			rotationChannelInfos[channel.targetIndex].push({&channel, duration, weight, time});
		}
	}

	// ------------------------------------------------------------------------

	// ----- ANIMATE TRANSLATION -----

	for (s32 boneIndex = 0; boneIndex < skeleton.bones.count(); ++boneIndex)
	{
		v3 totalTranslation 	= {};
		f32 totalAppliedWeight 	= 0;

		for(ChannelInfo const & channelInfo : translationChannelInfos[boneIndex])
		{
			TranslationChannel const & channel 	= *reinterpret_cast<TranslationChannel const *>(channelInfo.channel);

			v3 translation;

			if (channel.interpolationMode == INTERPOLATION_MODE_STEP)
			{
				s32 previousKeyframeIndex 	= previous_keyframe(channel.times, channelInfo.time);
				translation 				= channel.translations[previousKeyframeIndex]; 
			}
			else if (channel.interpolationMode == INTERPOLATION_MODE_LINEAR)
			{
				s32 previousKeyframeIndex 	= previous_keyframe(channel.times, channelInfo.time);
				s32 nextKeyframeIndex 		= (previousKeyframeIndex + 1) % channel.times.count();
				
				f32 relativeTime 			= get_relative_time(channel.times[previousKeyframeIndex],
																channel.times[nextKeyframeIndex],
																channelInfo.time,
																channelInfo.duration);

				translation = interpolate_v3(	channel.translations[previousKeyframeIndex],
													channel.translations[nextKeyframeIndex],
													relativeTime);
			}
			else
			{
				log_animation(0, "Unexpected interpolation mode: ", channel.interpolationMode);
				Assert(false);
			}


			totalAppliedWeight 	+= channelInfo.weight;
			totalTranslation 	+= translation * channelInfo.weight;
		}

		f32 defaultPoseWeight 	= 1.0f - totalAppliedWeight;
		totalTranslation 		+= skeleton.bones[boneIndex].boneSpaceDefaultTransform.position * defaultPoseWeight;

		skeleton.bones[boneIndex].boneSpaceTransform.position = totalTranslation;
	}

	// ------------------------------------------------------------------------

	// ----- ANIMATE ROTATION -----

	for (s32 boneIndex = 0; boneIndex < skeleton.bones.count(); ++boneIndex)
	{
		quaternion totalRotation = identity_quaternion;
		f32 totalAppliedWeight 	= 0;

		for (ChannelInfo const & channelInfo : rotationChannelInfos[boneIndex])
		{
			RotationChannel const & channel = *reinterpret_cast<RotationChannel const *>(channelInfo.channel);

			s32 previousKeyframeIndex	= previous_keyframe(channel.times, channelInfo.time);
			s32 nextKeyframeIndex 		= (previousKeyframeIndex + 1) % channel.times.count();
			f32 relativeTime 			= get_relative_time(channel.times[previousKeyframeIndex],
															channel.times[nextKeyframeIndex],
															channelInfo.time,
															channelInfo.duration);

			quaternion rotation;

			switch(channel.interpolationMode)
			{
				case INTERPOLATION_MODE_STEP:
				{
					rotation = channel.rotations[previousKeyframeIndex]; 
				} break;

				case INTERPOLATION_MODE_LINEAR:
				{
					rotation = interpolate_quaternion(	channel.rotations[previousKeyframeIndex],
														channel.rotations[nextKeyframeIndex],
														relativeTime);
				} break;

				default:
				{
					log_animation(0, "Unexpected interpolation mode: ", channel.interpolationMode);
					Assert(false);
				} break;
			}

			totalAppliedWeight 	+= channelInfo.weight;
			f32 relativeWeight 	= channelInfo.weight / totalAppliedWeight;
			totalRotation		= interpolate_quaternion(totalRotation, rotation, relativeWeight);
		}

		f32 defaultPoseWeight 	= 1.0f - totalAppliedWeight;
		totalRotation 			= interpolate_quaternion(	totalRotation,
															skeleton.bones[boneIndex].boneSpaceDefaultTransform.rotation,
															defaultPoseWeight);

		skeleton.bones[boneIndex].boneSpaceTransform.rotation = totalRotation;
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