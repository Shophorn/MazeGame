struct Keyframe
{
	float time;
	vector3 position;
};

// Data
struct Animation
{
	ArenaArray<Keyframe> keyframes;
};

// Data
struct AnimationClip
{
	ArenaArray<Animation> animations;
	float duration;
};

// Property/State
struct AnimationRig
{
	Handle<Transform3D> root;
	ArenaArray<Handle<Transform3D>> bones;
	ArenaArray<u64> currentBoneKeyframes;
};

struct Animator
{
	// Properties
	AnimationRig rig;
	float playbackSpeed			= 1.0f;

	// State
	bool32 isPlaying 			= false;
	AnimationClip * clip		= nullptr;
	float time;
};

internal AnimationRig
make_animation_rig(Handle<Transform3D> root, ArenaArray<Handle<Transform3D>> bones, ArenaArray<u64> currentBoneKeyframes)
{
	DEBUG_ASSERT(bones.count() == currentBoneKeyframes.count(), "Currently you must pass keyframe array with matching size to bones array. Sorry for inconvenience :)");

	AnimationRig result 
	{
		.root 					= root,
		.bones 					= bones,
		.currentBoneKeyframes 	= currentBoneKeyframes
	};
	return result;
}

internal void
update_animation_keyframes(Animation * animation, u64 * currentBoneKeyframe, float time)
{
	if (*currentBoneKeyframe < animation->keyframes.count()
		&& time > animation->keyframes[*currentBoneKeyframe].time)
	{
		*currentBoneKeyframe += 1;
		*currentBoneKeyframe = math::min(*currentBoneKeyframe, animation->keyframes.count());
	}	
}

internal void
update_animation_target(Animation * animation, Handle<Transform3D> target, u64 currentBoneKeyframe, float time)
{
	bool32 isBeforeFirstKeyFrame 	= currentBoneKeyframe == 0; 
	bool32 isAfterLastKeyFrame 		= currentBoneKeyframe >= animation->keyframes.count();

	if (isBeforeFirstKeyFrame)
	{
		auto currentKeyframe = animation->keyframes[currentBoneKeyframe];
		target->position = currentKeyframe.position;
	}
	else if(isAfterLastKeyFrame)
	{
		auto previousKeyframe = animation->keyframes[currentBoneKeyframe - 1];
		target->position = previousKeyframe.position;
	}
	else
	{
		auto previousKeyframe = animation->keyframes[currentBoneKeyframe - 1];
		auto currentKeyframe = animation->keyframes[currentBoneKeyframe];
		
		float previousKeyFrameTime 	= previousKeyframe.time;
		float currentKeyFrameTime 	= currentKeyframe.time;
		float relativeTime = (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		target->position = vector::interpolate(previousKeyframe.position, currentKeyframe.position, relativeTime); 
	}
}


internal AnimationClip
duplicate_animation_clip(MemoryArena * memoryArena, AnimationClip * original)
{
	AnimationClip result = 
	{
		.animations = duplicate_array(memoryArena, original->animations),
		.duration = original->duration
	};

	for (int childIndex = 0; childIndex < original->animations.count(); ++childIndex)
	{
		result.animations[childIndex].keyframes = duplicate_array(memoryArena, original->animations[childIndex].keyframes);
	}
	return result;
}

internal void
reverse_animation_clip(AnimationClip * clip)
{
	float duration = clip->duration;
	s32 childAnimationCount = clip->animations.count();

	for (int childIndex = 0; childIndex < childAnimationCount; ++childIndex)
	{
		reverse_arena_array(clip->animations[childIndex].keyframes);

		s32 keyframeCount = clip->animations[childIndex].keyframes.count();
		for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
		{
			float time = clip->animations[childIndex].keyframes[keyframeIndex].time; 
			clip->animations[childIndex].keyframes[keyframeIndex].time = duration - time;
		}
	}
}

internal float
compute_duration (ArenaArray<Animation> animations)
{
	// Note(Leo): We mitigate risks of copying in that we will just read these
	float duration = 0;
	for (	int animationIndex = 0;
			animationIndex < animations.count(); 
			++animationIndex)
	{
		for (	int keyframeIndex = 0; 
				keyframeIndex < animations[animationIndex].keyframes.count();
				++keyframeIndex)
		{
			float time = animations[animationIndex].keyframes[keyframeIndex].time;
			duration = math::max(duration, time);
		}
	}

	return duration;
}

internal AnimationClip
make_animation_clip (ArenaArray<Animation> animations)
{
	AnimationClip result = 
	{
		.animations = animations,
		.duration = compute_duration(animations)
	};
	return result;
}

internal void
update_animator_system(game::Input * input, ArenaArray<Handle<Animator>> animators)
{
	for (auto animator : animators)
	{
		if (animator->isPlaying == false)
			continue;


		float timeStep = animator->playbackSpeed * input->elapsedTime;
		animator->time += timeStep;


		for (int i = 0; i < animator->clip->animations.count(); ++i)
		{
			update_animation_keyframes(	&animator->clip->animations[i],
										&animator->rig.currentBoneKeyframes[i],
										animator->time);

			update_animation_target(	&animator->clip->animations[i],
										animator->rig.bones[i],
										animator->rig.currentBoneKeyframes[i],
										animator->time);
		}

		bool framesLeft = animator->time < animator->clip->duration;

		if (framesLeft == false)
		{
			animator->isPlaying = false;
			// std::cout << "[Animator]: AnimationClip complete\n";
		}
	}
}

internal Animator
make_animator(AnimationRig  rig)
{
	Animator result = 
	{
		.rig = rig
	};
	return result;
}


internal void
play_animation_clip(Handle<Animator> animator, AnimationClip * clip)
{
	animator->clip = clip;
	animator->time = 0.0f;
	animator->isPlaying = true;

	for (int boneIndex = 0; boneIndex < clip->animations.count(); ++boneIndex)
	{
		animator->rig.currentBoneKeyframes[boneIndex] = 0;
	}
}