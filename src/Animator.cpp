/*=============================================================================
Leo Tamminen

Animation System
=============================================================================*/

struct PositionKeyframe
{
	float 		time;
	v3 			position;
};

// struct RotationKeyframe
// {
// 	float time;
// 	quaternion rotation;
// };

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

// Data
// Note(Leo): This is for single bone's animation
struct BoneAnimation
{
	u32 boneIndex;

	Array<float> 			keyframeTimes;
	Array<v3> translations;

	Array<quaternion> 		rotations;

	ChannelType channelType;
	InterpolationMode interpolationMode;
};

// Data
// Note(Leo): This is for complete animation, with different keyframes for different bones
struct Animation
{
	Array<BoneAnimation> boneAnimations;
	float duration;

	bool loop = false;
};

// Property/State
struct AnimationRig
{
	Transform3D * 		root;
	Array<Transform3D*> bones;
	Array<u64> 			currentBonePositionKeyframes;
	Array<u64> 			currentBoneRotationKeyframes;
};

struct Animator
{
	// Properties
	AnimationRig rig;
	float playbackSpeed	= 1.0f;

	// State
	bool32 isPlaying 	= false;
	Animation * clip 	= nullptr;
	float time;
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
make_animation_rig(Transform3D * root, Array<Transform3D*> bones, Array<u64> currentBonePositionKeyframes)
{
	DEBUG_ASSERT(bones.count() == currentBonePositionKeyframes.count(), "Currently you must pass keyframe array with matching size to bones array. Sorry for inconvenience :)");

	AnimationRig result 
	{
		.root 							= root,
		.bones 							= std::move(bones),
		.currentBonePositionKeyframes 	= std::move(currentBonePositionKeyframes)
	};
	return result;
}

internal void
update_animation_keyframes(BoneAnimation * animation, u64 * currentBoneKeyframe, float time)
{
	if (*currentBoneKeyframe < animation->keyframeTimes.count()
		&& time > animation->keyframeTimes[*currentBoneKeyframe])
	{
		*currentBoneKeyframe += 1;
		*currentBoneKeyframe = math::min(*currentBoneKeyframe, animation->keyframeTimes.count());
	}	
}

internal void
update_animation_target(BoneAnimation * animation, Transform3D * target, u64 currentBoneKeyframe, float time)
{
	bool32 isBeforeFirstKeyFrame 	= currentBoneKeyframe == 0; 
	bool32 isAfterLastKeyFrame 		= currentBoneKeyframe >= animation->translations.count();

	if (isBeforeFirstKeyFrame)
	{
		auto currentPosition = animation->translations[currentBoneKeyframe];
		target->position = currentPosition;
	}
	else if(isAfterLastKeyFrame)
	{
		auto previousPosition = animation->translations[currentBoneKeyframe - 1];
		target->position = previousPosition;
	}
	else
	{
		auto previousPosition 		= animation->translations[currentBoneKeyframe - 1];
		auto currentPosition 		= animation->translations[currentBoneKeyframe];
		
		float previousKeyFrameTime 	= animation->keyframeTimes[currentBoneKeyframe - 1];
		float currentKeyFrameTime 	= animation->keyframeTimes[currentBoneKeyframe];
		float relativeTime 			= (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		target->position = vector::interpolate(previousPosition, currentPosition, relativeTime); 
	}
}

internal Animation
copy_animation(MemoryArena & memoryArena, Animation const & original)
{
	Animation result = 
	{
		.boneAnimations = allocate_array<BoneAnimation>(memoryArena, original.boneAnimations.count(), ALLOC_EMPTY),
		.duration = original.duration
	};

	for (int childIndex = 0; childIndex < original.boneAnimations.count(); ++childIndex)
	{
		result.boneAnimations.push({original.boneAnimations[childIndex].boneIndex, 
									copy_array(memoryArena, original.boneAnimations[childIndex].keyframeTimes),
									copy_array(memoryArena, original.boneAnimations[childIndex].translations),
									copy_array(memoryArena, original.boneAnimations[childIndex].rotations) });
	}
	return result;
}



internal void
reverse_animation_clip(Animation * clip)
{
	float duration = clip->duration;
	s32 childAnimationCount = clip->boneAnimations.count();

	for (int childIndex = 0; childIndex < childAnimationCount; ++childIndex)
	{
		reverse_BETTER_array(clip->boneAnimations[childIndex].keyframeTimes);
		reverse_BETTER_array(clip->boneAnimations[childIndex].translations);
		reverse_BETTER_array(clip->boneAnimations[childIndex].rotations);

		s32 keyframeCount = clip->boneAnimations[childIndex].translations.count();
		for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
		{
			float time = clip->boneAnimations[childIndex].keyframeTimes[keyframeIndex]; 
			clip->boneAnimations[childIndex].keyframeTimes[keyframeIndex] = duration - time;
		}
	}
}

internal float
compute_duration (Array<BoneAnimation> const & animations)
{
	float duration = 0;
	for (auto & animation : animations)
	{
		duration = math::max(duration, animation.keyframeTimes.last());
		// if (animation.translations.count() > 0)
		// 	duration = math::max(duration, animation.translations.last().time);

		// if (animation.rotations.count() > 0)

	}
	return duration;
}

internal Animation
make_animation (Array<BoneAnimation> animations)
{
	float duration = compute_duration(animations);

	Animation result = 
	{
		.boneAnimations = std::move(animations),
		.duration = duration
	};
	return result;
}

internal void
update_animator_system(game::Input * input, Array<Animator> & animators)
{
	for (auto & animator : animators)
	{
		if (animator.isPlaying == false)
			continue;

		float timeStep = animator.playbackSpeed * input->elapsedTime;
		animator.time += timeStep;

		for (auto & animation : animator.clip->boneAnimations)
		{
			u32 boneIndex = animation.boneIndex;

			update_animation_keyframes( &animation, &animator.rig.currentBonePositionKeyframes[boneIndex], animator.time);
			update_animation_target( &animation, animator.rig.bones[boneIndex], animator.rig.currentBonePositionKeyframes[boneIndex], animator.time);
		}

		bool framesLeft = animator.time < animator.clip->duration;

		if (framesLeft == false)
		{
			animator.isPlaying = false;
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
play_animation_clip(Animator * animator, Animation * clip)
{
	animator->clip = clip;
	animator->time = 0.0f;
	animator->isPlaying = true;

	for (auto & keyframe : animator->rig.currentBonePositionKeyframes)
	{
		keyframe = 0;
	}

	for(auto & keyframe : animator->rig.currentBoneRotationKeyframes)
	{
		keyframe = 0;
	}
}

s32 previous_rotation_keyframe(BoneAnimation const & animation, float time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframes time, so that it was actually the last. */

	// Note(Leo): Last keyframe from previous loop round
	if (time < animation.keyframeTimes[0])
	{
		return animation.keyframeTimes.count() - 1;
	}

	for (int i = 0; i < animation.keyframeTimes.count() - 1; ++i)
	{
		float thisTime = animation.keyframeTimes[i];
		float nextTime = animation.keyframeTimes[i + 1];

		if (thisTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return animation.keyframeTimes.count() - 1;
}

s32 previous_position_keyframe(BoneAnimation const & animation, float time)
{
	/* Note(Leo): In a looping animation, we have two possibilities to return 
	the last keyframe. First, if actual time is before the first actual keyframe,
	we then return last keyframe (from last loop round). The other is, if time is 
	more than that keyframes time, so that it was actually the last. */

	// Note(Leo): Last keyframe from previous loop round
	if (time < animation.keyframeTimes[0])
	{
		return animation.keyframeTimes.count() - 1;
	}

	for (int i = 0; i < animation.keyframeTimes.count() - 1; ++i)
	{
		float thisTime = animation.keyframeTimes[i];
		float nextTime = animation.keyframeTimes[i + 1];

		if (thisTime <= time && time <= nextTime)
		{
			return i;
		}
	}
	
	// Note(Leo): this loop rounds last keyframe 
	return animation.keyframeTimes.count() - 1;
}

quaternion get_rotation(BoneAnimation const & animation, float duration, float time)
{
	u32 previousKeyframeIndex = previous_rotation_keyframe(animation, time);
	u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % animation.rotations.count();

	// Get keyframes
	auto previousRotation 		= animation.rotations[previousKeyframeIndex];
	auto nextKeyframe 			= animation.rotations[nextKeyframeIndex];

	float previousTime = animation.keyframeTimes[previousKeyframeIndex];
	float nextTime = animation.keyframeTimes[nextKeyframeIndex];

	if (animation.interpolationMode == InterpolationMode::INTERPOLATION_MODE_STEP)
	{
		quaternion result = previousRotation;
		// logAnim(0) << result << "\n";

		return result;
	}
	
	float swagEpsilon = 0.000000001f; // Should this be in scale of frame time?

	// Note(Leo): these are times from previous so that we get [0 ... 1] range for interpolation
	float timeToNow = time - previousTime;
	if (timeToNow < swagEpsilon)
	{
		timeToNow = duration - timeToNow;
	} 

	float timeToNext = nextTime - previousTime;
	if (timeToNext < swagEpsilon)
	{
		timeToNext = duration - timeToNext;
	}
	float relativeTime = timeToNow / timeToNext;

	assert(previousRotation.is_unit_quaternion());
	assert(nextKeyframe.is_unit_quaternion());

	quaternion result = interpolate(previousRotation, nextKeyframe, relativeTime); 
	assert (result.is_unit_quaternion());
	return result;
}

v3 get_position(BoneAnimation const & animation, float duration, float time)
{
	u32 previousKeyframeIndex = previous_position_keyframe(animation, time);
	u32 nextKeyframeIndex = (previousKeyframeIndex + 1) % animation.translations.count();


	// Get keyframes
	v3 previousPosition = animation.translations[previousKeyframeIndex];
	v3 nextPosition 	= animation.translations[nextKeyframeIndex];
	
	float previousTime 	= animation.keyframeTimes[previousKeyframeIndex];
	float nextTime 		= animation.keyframeTimes[nextKeyframeIndex];

	float swagEpsilon = 0.000000001f; // Should this be in scale of frame time?

	// Note(Leo): these are times from previous so that we get [0 ... 1] range for interpolation
	float timeToNow = time - previousTime;
	if (timeToNow < swagEpsilon)
	{
		timeToNow = duration - timeToNow;
	} 

	float timeToNext = nextTime - previousTime;
	if (timeToNext < swagEpsilon)
	{
		timeToNext = duration - timeToNext;
	}

	float relativeTime = timeToNow / timeToNext;
	return vector::interpolate(previousPosition, nextPosition, relativeTime);
}

struct SkeletonAnimator
{
	Skeleton skeleton;

	float animationTime;
	Animation const * animation;
};

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

	for (auto & boneAnimation : animation.boneAnimations)
	{	
		auto & target = animator.skeleton.bones[boneAnimation.boneIndex].boneSpaceTransform;

		switch(boneAnimation.channelType)
		{
			case ANIMATION_CHANNEL_TRANSLATION:
				target.position = get_position(boneAnimation, animation.duration, animationTime);
				break;

			case ANIMATION_CHANNEL_ROTATION:
				target.rotation = get_rotation(boneAnimation, animation.duration, animationTime);
				break;

			case ANIMATION_CHANNEL_SCALE:
				logAnim(1) << FILE_ADDRESS << "Scale animation channel not implemented yet";
				break;

			default:
				assert(false);
				break;
		}
	}
}
