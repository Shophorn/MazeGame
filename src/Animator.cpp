struct Keyframe
{
	float time;
	Vector3 position;
};

struct Animation
{
	// Data
	ArenaArray<Keyframe> keyframes;
	
	// Properties
	Handle<Transform3D> target;
	
	// State
	uint64 currentKeyframeIndex = 0;

	const Keyframe & previous_keyframe () const { return keyframes[currentKeyframeIndex - 1]; }
	const Keyframe & current_keyframe () const { return keyframes[currentKeyframeIndex]; }
};

struct AnimationClip
{
	ArenaArray<Animation> animations;
	float duration;
};

internal void
update_animation_keyframes(Animation * animation, float time)
{
	if (animation->currentKeyframeIndex < animation->keyframes.count
		&& time > animation->current_keyframe().time)
	{
		animation->currentKeyframeIndex += 1;
		animation->currentKeyframeIndex = Min(animation->currentKeyframeIndex, animation->keyframes.count);
	}	
}

internal void
update_animation_target(Animation * animation, float time)
{
	bool32 isBeforeFirstKeyFrame 	= animation->currentKeyframeIndex == 0; 
	bool32 isAfterLastKeyFrame 		= animation->currentKeyframeIndex >= animation->keyframes.count;

	if (isBeforeFirstKeyFrame)
	{
		animation->target->position = animation->current_keyframe().position;
	}
	else if(isAfterLastKeyFrame)
	{
		animation->target->position = animation->previous_keyframe().position;
	}
	else
	{
		float previousKeyFrameTime 	= animation->previous_keyframe().time;
		float currentKeyFrameTime 	= animation->current_keyframe().time;
		float relativeTime = (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		animation->target->position = Interpolate(	animation->previous_keyframe().position,
													animation->current_keyframe().position,
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
		result.animations[childIndex].target = original->animations[childIndex].target;
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
	float playbackSpeed			= 1.0f;

	// State
	bool32 isPlaying 			= false;
	AnimationClip * clip 		= nullptr;
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
			update_animation_keyframes(&clip->animations[i], time);
			update_animation_target(&clip->animations[i], time);
		}

		bool framesLeft = time < clip->duration;

		if (framesLeft == false)
		{
			isPlaying = false;
			std::cout << "[Animator]: AnimationClip complete\n";
		}
	}
};

internal void
play_animation_clip(Animator * animator, AnimationClip * clip)
{
	animator->clip = clip;
	animator->time = 0.0f;
	animator->isPlaying = true;

	for (int animationIndex = 0; animationIndex < clip->animations.count; ++animationIndex)
	{
		clip->animations[animationIndex].currentKeyframeIndex = 0;
	}
}