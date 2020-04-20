struct FollowerController
{
	// References, must be set
	Transform3D * 	transform;
	Transform3D * 	targetTransform;
	s32 			motorInputIndex;

	// Properties, may be set
	f32 stopDistanceToTarget 	= 2;
	f32 jumpTimeInterval 		= 15.0f;
	f32 timeToWaitBeforeSit		= 5.0f;

	// State, should not be set
	f32 nextJumpTimeTimer;
	f32 timeWaitedBeforeSitting;
};


FollowerController make_follower_controller(Transform3D * 	transform,
											Transform3D * 	targetTransform,
											s32 			motorInputIndex)
{
	FollowerController controller = {transform, targetTransform, motorInputIndex};
	controller.nextJumpTimeTimer = RandomRange(0, controller.jumpTimeInterval);
	return controller;
}

void update_follower_input(	FollowerController 			& controller,
							Array<CharacterInput> 	& motorInputs,
							f32 elapsedTime)
{
	v3 toTarget 			= controller.targetTransform->position - controller.transform->position;
	toTarget.z = 0;
	float distanceToTarget 	= toTarget.magnitude();

	distanceToTarget = math::clamp(distanceToTarget - controller.stopDistanceToTarget, 0.0f, 1.0f);

	v3 inputVector = distanceToTarget < 0.00001f
					? v3 {0, 0, 0}
					: toTarget.normalized() * distanceToTarget;



	bool32 jumpInput = false;
	bool32 crouchInput = false;

	if (distanceToTarget < 0.00001f)
	{
		controller.timeWaitedBeforeSitting += elapsedTime;

		if (controller.timeWaitedBeforeSitting > controller.timeToWaitBeforeSit)
		{
			crouchInput = true;
		}

		controller.nextJumpTimeTimer = controller.jumpTimeInterval;
	}
	else
	{
		controller.timeWaitedBeforeSitting = 0;
	}
	
	if (distanceToTarget > 0.99999f)
	{
		controller.nextJumpTimeTimer -= elapsedTime;
		if (controller.nextJumpTimeTimer < 0.0f)
		{
			controller.nextJumpTimeTimer = controller.jumpTimeInterval;
			jumpInput = true;
		}
	}




	motorInputs[controller.motorInputIndex] = {inputVector, jumpInput, crouchInput};
}