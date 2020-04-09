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

enum { NOT_FOUND = (u32)-1 };
internal u32 find_channel(Animation const & animation, s32 targetIndex, ChannelType type)
{
	for (int channelIndex = 0; channelIndex < animation.channels.count(); ++channelIndex)
	{
		if (animation.channels[channelIndex].targetIndex == targetIndex
			&& animation.channels[channelIndex].type == type)
		{
			return channelIndex;
		}
	}
	return NOT_FOUND;
}

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
	Transform3D defaultPose;
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
 	// float 				animationTime;
	// Animation const * 	animation;

	Animation const * 	animations[2];
	float 				animationTimes [2];
	float 				animationWeights [2] = {0.5f, 0.5f};
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


float get_relative_time(AnimationChannel const & 	channel,
						s32 						previousKeyframeIndex,
						s32 						nextKeyframeIndex,
						float 						animationTime,
						float 						animationDuration)
{
	float previousTime 	= channel.times[previousKeyframeIndex];
	float nextTime 		= channel.times[nextKeyframeIndex];

	float swagEpsilon = 0.000000001f; // Should this be in scale of frame time?

	// Note(Leo): these are times from previous so that we get [0 ... 1] range for interpolation
	float timeToNow = animationTime - previousTime;
	if (timeToNow < swagEpsilon)
	{
		timeToNow = animationDuration - timeToNow;
	} 

	float timeToNext = nextTime - previousTime;
	if (timeToNext < swagEpsilon)
	{
		timeToNext = animationDuration - timeToNext;
	}
	float relativeTime = timeToNow / timeToNext;

	return relativeTime;
}

void update_skeleton_animator(SkeletonAnimator & animator, float elapsedTime)
{
	// Reset
	for (Bone & bone : animator.skeleton.bones)
	{
		bone.boneSpaceTransform = {};
	}

	// for bone in skeleton:	
	// 	rotation1 = get_rotation(animation1, bone, time);
	// 	rotation2 = get_rotation(animation2, bone, time);
	// 	bone.rotation = interpolate(rotation1, rotation2, weight1 / (weight1 + weight2));
	
	/*This procedure:
		- advances animation time
		- updates skeleton's bones
	*/
	if (animator.animations[0] == nullptr)
		return;

	quaternion rotations_0 [50];
	quaternion rotations_1 [50];
	float weights_for_0 [50];

	for (int i = 0; i < 2; ++i)
	{
		if (animator.animations[i] == nullptr)
		{
			continue;
		}

		Animation const & animation = *animator.animations[i];
		float & animationTime = animator.animationTimes[i];
		float animationWeight = animator.animationWeights[i];

		// ------------------------------------------------------------------------

		animationTime += elapsedTime;
		animationTime = loop_time(animationTime, animation.duration);

		// for (auto & bone : animator.skeleton.bones)
		for (int boneIndex = 0; boneIndex < animator.skeleton.bones.count(); ++boneIndex)
		{
			auto & bone = animator.skeleton.bones[boneIndex];

			quaternion rotation = bone.defaultPose.rotation;
			u32 channelIndex = find_channel(animation, boneIndex, ANIMATION_CHANNEL_ROTATION);
			if (channelIndex != NOT_FOUND)
			{
				auto const & channel = animation.channels[channelIndex];
					
				u32 previousKeyframeIndex = previous_keyframe(channel.times, animationTime);

				if (channel.interpolationMode == INTERPOLATION_MODE_STEP)
				{
					rotation = channel.rotations[previousKeyframeIndex]; 
				}
				else
				{
					u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % channel.times.count();
					float relativeTime = get_relative_time(	channel, 
															previousKeyframeIndex, nextKeyframeIndex,
															animationTime, animation.duration);

					rotation = interpolate(	channel.rotations[previousKeyframeIndex],
													channel.rotations[nextKeyframeIndex],
													relativeTime);
				}
			}
			// bone.boneSpaceTransform.rotation = interpolate(quaternion::identity(), rotation, animationWeight) * bone.boneSpaceTransform.rotation;
			if (i == 0)
			{
				rotations_0[boneIndex] = rotation;
				weights_for_0[boneIndex] = animationWeight;
			}
			else if (i == 1)
			{
				rotations_1[boneIndex] = rotation;
			}

			v3 translation = bone.defaultPose.position;
			channelIndex = find_channel(animation, boneIndex, ANIMATION_CHANNEL_TRANSLATION);
			if (channelIndex != NOT_FOUND)
			{
				auto const & channel = animation.channels[channelIndex];
				u32 previousKeyframeIndex = previous_keyframe(channel.times, animationTime);

				if (channel.interpolationMode == INTERPOLATION_MODE_STEP)
				{
					translation = channel.translations[previousKeyframeIndex]; 
				}
				else
				{
					u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % channel.times.count();
					float relativeTime = get_relative_time(	channel, 
															previousKeyframeIndex, nextKeyframeIndex,
															animationTime, animation.duration);

					translation = vector::interpolate(	channel.translations[previousKeyframeIndex],
													channel.translations[nextKeyframeIndex],
													relativeTime);
				}
			}
			bone.boneSpaceTransform.position += translation * animationWeight;
		}
	}

	for (int i = 0; i < animator.skeleton.bones.count(); ++i)
	{
		if (weights_for_0[i] < 0.99999f)
			animator.skeleton.bones[i].boneSpaceTransform.rotation = interpolate(rotations_0[i], rotations_1[i], weights_for_0[i]);
		else
			animator.skeleton.bones[i].boneSpaceTransform.rotation = rotations_0[i];
	}
}
