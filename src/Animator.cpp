struct Keyframe
{
	float time;
	Vector3 position;
};

struct Animation
{
	Handle<Transform3D> target;
	ArenaArray<Keyframe> keyframes;
	
	int32 currentKeyFrameIndex = 0;

	Keyframe & PreviousKeyFrame () { return keyframes[currentKeyFrameIndex - 1]; }
	Keyframe & CurrentKeyFrame 	() { return keyframes[currentKeyFrameIndex]; }
	Keyframe & LastKeyFrame 	() { return keyframes[keyframes.count - 1]; }
};


internal Vector3
get_position(Animation * animation, float time)
{
	if (time > animation->keyframes[animation->currentKeyFrameIndex].time)
	{
		animation->currentKeyFrameIndex += 1;
		animation->currentKeyFrameIndex = Min(animation->currentKeyFrameIndex, (int32)animation->keyframes.count - 1);
	}

	bool32 isBeforeFirstKeyFrame 	= animation->currentKeyFrameIndex == 0; 
	bool32 isAfterLastKeyFrame 		= (&animation->CurrentKeyFrame() == &animation->LastKeyFrame())
									&& (time > animation->keyframes[animation->currentKeyFrameIndex].time);

	// Note(Leo): only interpolate if we have keyframes on both side of current time
	bool32 interpolatePosition 		= (isBeforeFirstKeyFrame == false
									&& isAfterLastKeyFrame == false);

	Vector3 position;
	if (interpolatePosition)
	{
		float previousKeyFrameTime 	= animation->PreviousKeyFrame().time;
		float currentKeyFrameTime 	= animation->CurrentKeyFrame().time;
		float relativeTime = (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

		position = Interpolate(	animation->PreviousKeyFrame().position,
								animation->CurrentKeyFrame().position,
								relativeTime); 

	}
	else
	{
		position = animation->CurrentKeyFrame().position;
	}

	return position;
}

struct AnimationClip
{
	// Data
	ArenaArray<Animation> children;
	
	// Properties
	float duration;

	// State
	float time;
};

// Note(Leo): returns TRUE if animation IS NOT finished
internal bool32
Advance(AnimationClip * animation, float elapsedTime)
{
	animation->time += elapsedTime;

	for (int childIndex = 0; childIndex < animation->children.count; ++childIndex)
	{
		Vector3 position = get_position(&animation->children[childIndex], animation->time);
		animation->children[childIndex].target->position = position;
	}

	return animation->time < animation->duration;
}

internal void
Reset(AnimationClip * animation)
{
	animation->time = 0.0f;
}

internal AnimationClip
reverse_animation_clip(MemoryArena * memoryArena, AnimationClip * original)
{
	float duration = original->duration;
	int32 childAnimationCount = original->children.count;
	
	auto children = push_array<Animation>(memoryArena, childAnimationCount, true);

	for (int childIndex = 0; childIndex < childAnimationCount; ++childIndex)
	{
		children[childIndex].target = original->children[childIndex].target;

		int32 keyframeCount = original->children[childIndex].keyframes.count;
		children[childIndex].keyframes = push_array<Keyframe>(memoryArena, keyframeCount);
		for (int keyframeIndex = 0; keyframeIndex < keyframeCount; ++keyframeIndex)
		{
			int originalKeyframeIndex = keyframeCount - 1 - keyframeIndex;
			Keyframe originalKeyframe = original->children[childIndex].keyframes[originalKeyframeIndex];

			float time = duration - originalKeyframe.time;
			Vector3 position = originalKeyframe.position;

			children[childIndex].keyframes[keyframeIndex] = {time, position};
		}
	}

	AnimationClip result = 
	{
		.children = children,
		.duration = duration
	};

	return result;
}

internal float
ComputeDuration (AnimationClip * animation)
{
	float duration = 0;
	for (	int childIndex = 0;
			childIndex < animation->children.count; 
			++childIndex)
	{
		for (	int keyframeIndex = 0; 
				keyframeIndex < animation->children[childIndex].keyframes.count;
				++keyframeIndex)
		{
			float time = animation->children[childIndex].keyframes[keyframeIndex].time;
			duration = Max(duration, time);
		}
	}

	return duration;
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
		.children = animations,
		.duration = compute_duration(animations)
	};
	return result;
}

struct Animator
{
	// Properties
	float playbackSpeed		= 1.0f;

	// State
	bool32 isPlaying 		= false;
	AnimationClip * animation 	= nullptr;


	void
	Play(AnimationClip * animation)
	{
		// Todo(Leo): Add blending etc. here :)

		this->animation = animation;
		Reset(animation);
		isPlaying = true;
	}

	void
	Update(game::Input * input)
	{
		if (isPlaying == false)
			return;

		float timeStep = playbackSpeed * input->elapsedTime;
		bool framesLeft = Advance(animation, timeStep);

		if (framesLeft == false)
		{
			isPlaying = false;
			std::cout << "[Animator]: AnimationClip complete\n";
		}
	}
};