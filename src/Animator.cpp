struct Animator
{
	bool32 playing = true;

	int32 currentTargetIndex;

	BETTERArenaArray<Transform3D *> 	targets;
	BETTERArenaArray<Vector3> 			localStartPositions;
	BETTERArenaArray<Vector3>			localEndPositions;

	float speed;

	float time = 0;

	void Update(game::Input * input)
	{
		if (playing == false)
			return;

		float step 				= speed * input->elapsedTime;
		time 					= Clamp(time + step, 0.0f, 1.0f);

		targets[currentTargetIndex]->position  = Interpolate(	localStartPositions[currentTargetIndex],
																localEndPositions[currentTargetIndex],
																time);

		std::cout << "[ANIMATOR]: current index " << currentTargetIndex << "\n";

		if (time == 1.0f)
		{
			currentTargetIndex++;
			time = 0.0f;

			if (currentTargetIndex >= targets.count)
			{
				playing = false;
			}
		}
	}
};