struct Keyframe
{
	// Handle<Transform3D> target;
	float time;
	Vector3 position;
};

struct ChildAnimation
{
	Handle<Transform3D> target;
	ArenaArray<Keyframe> keyframes;
	
	int32 currentKeyFrameIndex = 0;

	Keyframe & PreviousKeyFrame () { return keyframes[currentKeyFrameIndex - 1]; }
	Keyframe & CurrentKeyFrame 	() { return keyframes[currentKeyFrameIndex]; }
	Keyframe & LastKeyFrame 	() { return keyframes[keyframes.count - 1]; }
};


internal Vector3
GetPosition(ChildAnimation * animation, float time)
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
	bool32 interpolatePosition = 	(isBeforeFirstKeyFrame == false
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

struct Animation
{
	// Data
	ArenaArray<ChildAnimation> children;
	
	// Properties
	float duration;

	// State
	float time;
};

// Note(Leo): returns TRUE if animation IS NOT finished
internal bool32
Advance(Animation * animation, float elapsedTime)
{
	animation->time += elapsedTime;

	for (int childIndex = 0; childIndex < animation->children.count; ++childIndex)
	{
		Vector3 position = GetPosition(&animation->children[childIndex], animation->time);
		animation->children[childIndex].target->position = position;
	}

	return animation->time < animation->duration;
}

struct Animator
{
	// Properties
	float playbackSpeed		= 1.0f;

	// State
	bool32 isPlaying 		= false;
	Animation * animation 	= nullptr;

	void
	Play(Animation * animation)
	{
		// Todo(Leo): Add blending etc. here :)

		this->animation = animation;
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
			std::cout << "[Animator]: Animation complete\n";
		}
	}
};