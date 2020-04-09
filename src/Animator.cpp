/*=============================================================================
Leo Tamminen

Animation System
=============================================================================*/

enum InterpolationMode
{
	INTERPOLATION_MODE_STEP,
	INTERPOLATION_MODE_LINEAR,
	INTERPOLATION_MODE_CUBICSPLINE,

	INTERPOLATION_MODE_COUNT
};

enum ChannelType
{
	ANIMATION_CHANNEL_TRANSLATION,
	ANIMATION_CHANNEL_ROTATION,
	ANIMATION_CHANNEL_SCALE,

	ANIMATION_CHANNEL_COUNT
};

struct AnimationChannel
{
	u32 				targetIndex;

	Array<float> 		times;
	Array<v3> 			translations;
	Array<quaternion> 	rotations;

	ChannelType 		type;
	InterpolationMode 	interpolationMode;

	int keyframe_count() const { return times.count(); }
};

// Data
// Note(Leo): This is for complete animation, with different keyframes for different bones
struct Animation
{
	Array<AnimationChannel> channels;
	float 					duration;

	bool 					loop = false;
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
	float playbackSpeed	= 1.0f;

	// State
	float 				time;
	Animation const * 	animation;
};

struct Bone
{
	// State
	Transform3D boneSpaceTransform;

	// Properties
	m44 		inverseBindMatrix;
	u32 		parent;
	bool32 		isRoot;
	SmallString name;
};

struct Skeleton
{
	Array<Bone> bones;
};

struct SkeletonAnimator
{
	Skeleton 			skeleton;
 	float 				animationTime;
	Animation const * 	animation;

	Animation const * 	animations[2];
	float 				animationTimes [2];
	float 				animationWeights [2];
};

v3 get_model_space_position(Skeleton const & skeleton, u32 boneIndex)
{
	v3 position = skeleton.bones[boneIndex].boneSpaceTransform.position;
	if (skeleton.bones[boneIndex].isRoot == false)
	{
		position += get_model_space_position(skeleton, skeleton.bones[boneIndex].parent);
	}
	return position;
}

m44 get_model_space_transform(Skeleton const & skeleton, u32 boneIndex)
{
	Bone const & bone = skeleton.bones[boneIndex];

	m44 transform = get_matrix(bone.boneSpaceTransform);
	if (bone.isRoot == false)
	{
		transform = get_model_space_transform(skeleton, bone.parent) * transform; 
	}
	return transform;
}

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
copy_animation(MemoryArena & memoryArena, Animation const & original)
{
	Animation result = 
	{
		.channels = allocate_array<AnimationChannel>(memoryArena, original.channels.count(), ALLOC_EMPTY),
		.duration = original.duration
	};

	for (int childIndex = 0; childIndex < original.channels.count(); ++childIndex)
	{
		result.channels.push({original.channels[childIndex].targetIndex, 
									copy_array(memoryArena, original.channels[childIndex].times),
									copy_array(memoryArena, original.channels[childIndex].translations),
									copy_array(memoryArena, original.channels[childIndex].rotations) });
	}
	return result;
}

internal void
reverse_animation_clip(Animation & animation)
{
	for (AnimationChannel & channel : animation.channels)
	{
		switch(channel.type)
		{
			case ANIMATION_CHANNEL_TRANSLATION:
				reverse_array(channel.translations);
				break;

			case ANIMATION_CHANNEL_ROTATION:
				reverse_array(channel.rotations);
				break;

			default:
				logDebug(0) << FILE_ADDRESS << "Invalid or unimplemented animation channel type: " << channel.type;
				// Note(Leo): Continue here instead of break, we probably should not touch times either.
				continue;
		}

		reverse_array(channel.times);

		for(float & time : channel.times)
		{
			time = animation.duration - time;
		}
	}
}

internal float
compute_duration (Array<AnimationChannel> const & channels)
{
	float duration = 0;
	for (auto & channel : channels)
	{
		duration = math::max(duration, channel.times.last());
	}
	return duration;
}

internal Animation
make_animation (Array<AnimationChannel> channels)
{
	float duration = compute_duration(channels);

	Animation result = 
	{
		.channels = std::move(channels),
		.duration = duration
	};
	return result;
}

s32 previous_keyframe(Array<float> const & keyframes, float time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframes time, so that it was actually the last. */

	// Note(Leo): Last keyframe from previous loop round
	if (time < keyframes[0])
	{
		return keyframes.count() - 1;
	}

	for (int i = 0; i < keyframes.count() - 1; ++i)
	{
		float previousTime = keyframes[i];
		float nextTime = keyframes[i + 1];

		if (previousTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return keyframes.count() - 1;
}


internal void
update_animator_system(game::Input * input, Array<Animator> & animators)
{
	for (auto & animator : animators)
	{
		if (animator.animation == nullptr)
			continue;

		float timeStep = animator.playbackSpeed * input->elapsedTime;
		animator.time += timeStep;

		for (auto const & channel : animator.animation->channels)
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

				float previousKeyFrameTime 	= channel.times[previousKeyframe];
				float currentKeyFrameTime 	= channel.times[nextKeyframe];
				float relativeTime 			= (animator.time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

				target.position = vector::interpolate(	channel.translations[previousKeyframe],
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



float loop_time(float time, float duration)
{
	if (duration == 0)
		time = 0;
	else
		time = modulo(time, duration);

	return time;
}

void update_skeleton_animator(SkeletonAnimator & animator, float elapsedTime)
{
	/*This procedure:
		- advances animation time
		- updates skeleton's bones
	*/
	if (animator.animation == nullptr)
		return;

	Animation const & animation = *animator.animation;
	float & animationTime = animator.animationTime;

	// ------------------------------------------------------------------------

	animationTime += elapsedTime;
	animationTime = loop_time(animationTime, animator.animation->duration);

	for (auto & channel : animation.channels)
	{	
		auto & target = animator.skeleton.bones[channel.targetIndex].boneSpaceTransform;

		u32 previousKeyframeIndex = previous_keyframe(channel.times, animationTime);

		if (channel.interpolationMode == INTERPOLATION_MODE_STEP)
		{
			switch(channel.type)
			{
				case ANIMATION_CHANNEL_TRANSLATION:
					target.position = channel.translations[previousKeyframeIndex];
					break;

				case ANIMATION_CHANNEL_ROTATION: 
					target.rotation = channel.rotations[previousKeyframeIndex]; 
					break;

				default:
					logDebug(0) << FILE_ADDRESS << "Invalid or unimplemented animation channel: " << channel.type;
			}
		}
		else
		{
			u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % channel.times.count();

			float previousTime 	= channel.times[previousKeyframeIndex];
			float nextTime 		= channel.times[nextKeyframeIndex];

			float swagEpsilon = 0.000000001f; // Should this be in scale of frame time?

			// Note(Leo): these are times from previous so that we get [0 ... 1] range for interpolation
			float timeToNow = animationTime - previousTime;
			if (timeToNow < swagEpsilon)
			{
				timeToNow = animation.duration - timeToNow;
			} 

			float timeToNext = nextTime - previousTime;
			if (timeToNext < swagEpsilon)
			{
				timeToNext = animation.duration - timeToNext;
			}
			float relativeTime = timeToNow / timeToNext;

			switch(channel.type)
			{
				case ANIMATION_CHANNEL_TRANSLATION: {
					target.position = vector::interpolate(	channel.translations[previousKeyframeIndex],
															channel.translations[nextKeyframeIndex],
															relativeTime);
				} break;

				case ANIMATION_CHANNEL_ROTATION: {
					target.rotation = interpolate(	channel.rotations[previousKeyframeIndex],
													channel.rotations[nextKeyframeIndex],
													relativeTime);
				} break;

				case ANIMATION_CHANNEL_SCALE:
					logAnim(1) << FILE_ADDRESS << "Scale animation channel not implemented yet";
					break;

				default:
					assert(false);
					break;
			}
		}
	}
}
