struct Animator
{
	bool32 playing = true;

	int32 currentTargetIndex;


	constexpr static int32 stageCount = 6;
	Transform3D * targets[stageCount];

	Vector3 localStartPosition;
	Vector3 localEndPosition;

	float speed;

	float time = 0;

	void Update(game::Input * input)
	{
		if (playing == false)
			return;

		float step 				= speed * input->elapsedTime;
		time 					= Clamp(time + step, 0.0f, 1.0f);

		targets[currentTargetIndex]->position  = Interpolate(localStartPosition, localEndPosition, time);

		if (currentTargetIndex == 0)
		{
			targets[currentTargetIndex]->position.z -= 1.0f;
		}

		if (time == 1.0f)
		{
			currentTargetIndex++;
			time = 0.0f;

			if (currentTargetIndex >= stageCount)
			{
				playing = false;
			}
		}
	}
};