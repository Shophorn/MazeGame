struct Keyframe
{
	float time;
	Vector3 position;
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
	ArenaArray<uint64> currentBoneKeyframes;
};

internal AnimationRig
make_animation_rig(Handle<Transform3D> root, ArenaArray<Handle<Transform3D>> bones, ArenaArray<uint64> currentBoneKeyframes)
{
	MAZEGAME_ASSERT(bones.count == currentBoneKeyframes.count, "Currently you must pass keyframe array with matching size to bones array. Sorry for inconvenience :)");

	AnimationRig result 
	{
		.root 					= root,
		.bones 					= bones,
		.currentBoneKeyframes 	= currentBoneKeyframes
	};
	return result;
}

internal void
update_animation_keyframes(const Animation * animation, uint64 * currentBoneKeyframe, float time)
{
	if (*currentBoneKeyframe < animation->keyframes.count
		&& time > animation->keyframes[*currentBoneKeyframe].time)
	{
		*currentBoneKeyframe += 1;
		*currentBoneKeyframe = Min(*currentBoneKeyframe, animation->keyframes.count);
	}	
}

internal void
update_animation_target(const Animation * animation, Handle<Transform3D> target, uint64 currentBoneKeyframe, float time)
{
	bool32 isBeforeFirstKeyFrame 	= currentBoneKeyframe == 0; 
	bool32 isAfterLastKeyFrame 		= currentBoneKeyframe >= animation->keyframes.count;

	if (isBeforeFirstKeyFrame)
	{
		auto current_keyframe = animation->keyframes[currentBoneKeyframe];
		target->position = current_keyframe.position;
	}
	else if(isAfterLastKeyFrame)
	{
		auto previous_keyframe = animation->keyframes[currentBoneKeyframe - 1];
		target->position = previous_keyframe.position;
	}
	else
	{
		auto previous_keyframe = animation->keyframes[currentBoneKeyframe - 1];
		auto current_keyframe = animation->keyframes[currentBoneKeyframe];
		
		float previousKeyFrameTime 	= previous_keyframe.time;
		float currentKeyFrameTime 	= current_keyframe.time;
		float relativeTime = (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		target->position = Interpolate(	previous_keyframe.position,
													current_keyframe.position,
													relativeTime); 
	}
}


internal AnimationClip
duplicate_animation_clip(MemoryArena * memoryArena, AnimationClip * original)
{
	AnimationClip result = 
	{
		.animations = duplicate_arena_array(memoryArena, &original->animations),
		.duration = original->duration
	};

	for (int childIndex = 0; childIndex < original->animations.count; ++childIndex)
	{
		result.animations[childIndex].keyframes = duplicate_arena_array(memoryArena, &original->animations[childIndex].keyframes);
	}
	return result;
}

internal void
reverse_animation_clip(AnimationClip * clip)
{
	float duration = clip->duration;
	int32 childAnimationCount = clip->animations.count;

	for (int childIndex = 0; childIndex < childAnimationCount; ++childIndex)
	{
		reverse_arena_array(&clip->animations[childIndex].keyframes);

		int32 keyframeCount = clip->animations[childIndex].keyframes.count;
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
			animationIndex < animations.count; 
			++animationIndex)
	{
		for (	int keyframeIndex = 0; 
				keyframeIndex < animations[animationIndex].keyframes.count;
				++keyframeIndex)
		{
			float time = animations[animationIndex].keyframes[keyframeIndex].time;
			duration = Max(duration, time);
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

struct Animator
{
	// Properties
	AnimationRig * rig;
	float playbackSpeed			= 1.0f;

	// State
	bool32 isPlaying 			= false;
	const AnimationClip * clip	= nullptr;
	float time;

	void
	Update(game::Input * input)
	{
		if (isPlaying == false)
			return;

		float timeStep = playbackSpeed * input->elapsedTime;
		time += timeStep;

		for (int i = 0; i < clip->animations.count; ++i)
		{
			update_animation_keyframes(&clip->animations[i], &rig->currentBoneKeyframes[i], time);
			update_animation_target(&clip->animations[i], rig->bones[i], rig->currentBoneKeyframes[i], time);
		}

		bool framesLeft = time < clip->duration;

		if (framesLeft == false)
		{
			isPlaying = false;
			std::cout << "[Animator]: AnimationClip complete\n";
		}
	}
};

internal Animator
make_animator(AnimationRig * rig)
{
	Animator result = 
	{
		.rig = rig
	};
	return result;
}


internal void
play_animation_clip(Animator * animator, const AnimationClip * clip)
{
	animator->clip = clip;
	animator->time = 0.0f;
	animator->isPlaying = true;

	for (int boneIndex = 0; boneIndex < clip->animations.count; ++boneIndex)
	{
		animator->rig->currentBoneKeyframes[boneIndex] = 0;
	}
}