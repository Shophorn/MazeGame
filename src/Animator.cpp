/*=============================================================================
Leo Tamminen

Animation System
=============================================================================*/

struct PositionKeyframe
{
	float 		time;
	v3 			position;
};

struct RotationKeyframe
{
	float time;
	quaternion rotation;
};

// Data
// Note(Leo): This is for single bone's animation
struct BoneAnimation
{
	u32 boneIndex;
	Array<PositionKeyframe> positions;
	Array<RotationKeyframe> rotations;
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
	Transform3D boneSpaceTransform;
	m44 		inverseBindMatrix;
	u32 		parent;
	bool32 		isRoot;

	SmallString name;
};

struct Skeleton
{
	Array<Bone> bones;
};

struct BoneDescription
{
	m44 		inverseBindMatrix;
	u32 		parent;
	bool32 		isRoot;
	SmallString name;
};

struct SkeletonDescription
{
	Array<BoneDescription> bones;	
};

struct SkeletonAnimator
{
	// Property
	SkeletonDescription const * const skeleton = nullptr;

	// State
	Array<Transform3D> boneTransforms;

	Animation * animation;
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
		transform = matrix::multiply(get_model_space_transform(skeleton, bone.parent), transform); 
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
	if (*currentBoneKeyframe < animation->positions.count()
		&& time > animation->positions[*currentBoneKeyframe].time)
	{
		*currentBoneKeyframe += 1;
		*currentBoneKeyframe = math::min(*currentBoneKeyframe, animation->positions.count());
	}	
}

internal void
update_animation_target(BoneAnimation * animation, Transform3D * target, u64 currentBoneKeyframe, float time)
{
	bool32 isBeforeFirstKeyFrame 	= currentBoneKeyframe == 0; 
	bool32 isAfterLastKeyFrame 		= currentBoneKeyframe >= animation->positions.count();

	if (isBeforeFirstKeyFrame)
	{
		auto currentKeyframe = animation->positions[currentBoneKeyframe];
		target->position = currentKeyframe.position;
	}
	else if(isAfterLastKeyFrame)
	{
		auto previousKeyframe = animation->positions[currentBoneKeyframe - 1];
		target->position = previousKeyframe.position;
	}
	else
	{
		auto previousKeyframe 		= animation->positions[currentBoneKeyframe - 1];
		auto currentKeyframe 		= animation->positions[currentBoneKeyframe];
		
		float previousKeyFrameTime 	= previousKeyframe.time;
		float currentKeyFrameTime 	= currentKeyframe.time;
		float relativeTime 			= (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		target->position = vector::interpolate(previousKeyframe.position, currentKeyframe.position, relativeTime); 
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
									copy_array(memoryArena, original.boneAnimations[childIndex].positions),
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
		reverse_BETTER_array(clip->boneAnimations[childIndex].positions);

		s32 keyframeCount = clip->boneAnimations[childIndex].positions.count();
		for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
		{
			float time = clip->boneAnimations[childIndex].positions[keyframeIndex].time; 
			clip->boneAnimations[childIndex].positions[keyframeIndex].time = duration - time;
		}
	}
}

internal float
compute_duration (Array<BoneAnimation> const & animations)
{
	float duration = 0;
	for (auto & animation : animations)
	{
		if (animation.positions.count() > 0)
			duration = math::max(duration, animation.positions.last().time);

		if (animation.rotations.count() > 0)
			duration = math::max(duration, animation.rotations.last().time);

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