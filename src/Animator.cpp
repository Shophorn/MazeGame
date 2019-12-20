struct Keyframe
{
	Handle<Transform3D> target;
	float time;
	Vector3 position;
};

struct Animation
{
	// Handle<Transform3D> target;
	
	// Todo(Leo): Must be kept sorted
	ArenaArray<Keyframe> keyframes;
	int32 currentKeyFrameIndex = 0;

	Keyframe & PreviousKeyFrame () { return keyframes[currentKeyFrameIndex - 1]; }
	Keyframe & CurrentKeyFrame 	() { return keyframes[currentKeyFrameIndex]; }
	Keyframe & LastKeyFrame 	() { return keyframes[keyframes.count - 1]; }
};

struct Animator
{
	bool32 playing = true;
	float playbackSpeed;
	float time = 0;

	Animation animation;

	void Update(game::Input * input)
	{
		if (playing == false)
			return;

		float timeStep = playbackSpeed * input->elapsedTime;
		time = time + timeStep;

		if (time > animation.keyframes[animation.currentKeyFrameIndex].time)
		{
			animation.currentKeyFrameIndex += 1;
			animation.currentKeyFrameIndex = Min(animation.currentKeyFrameIndex, (int32)animation.keyframes.count - 1);
		}

		bool32 isBeforeFirstKeyFrame 	= animation.currentKeyFrameIndex == 0; 
		bool32 isAfterLastKeyFrame 		= (&animation.CurrentKeyFrame() == &animation.LastKeyFrame())
										&& (time > animation.keyframes[animation.currentKeyFrameIndex].time);

		/* Note(Leo): Do not skip this animation frame anyway, since we want to end
		at the actual position of the keyframe, and not the interpolated one from
		previous frame.*/
		if (isAfterLastKeyFrame)
		{
			// Todo(Leo): Add looping option

			playing = false;
		}


		// Note(Leo): only interpolate if we have keyframes on both side of current time
		bool32 interpolatePosition = 	(isBeforeFirstKeyFrame == false
										&& isAfterLastKeyFrame == false);

		Vector3 position;
		if (interpolatePosition)
		{
			float previousKeyFrameTime 	= animation.PreviousKeyFrame().time;
			float currentKeyFrameTime 	= animation.CurrentKeyFrame().time;
			float relativeTime = (time - previousKeyFrameTime) / (currentKeyFrameTime - previousKeyFrameTime);

			position = Interpolate(	animation.PreviousKeyFrame().position,
									animation.CurrentKeyFrame().position,
									relativeTime); 

		}
		else
		{
			position = animation.CurrentKeyFrame().position;
		}

		animation.CurrentKeyFrame().target->position = position;
	}
};