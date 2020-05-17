struct FollowerController
{
	// References, must be set
	Transform3D * 	transform;
	Transform3D * 	targetTransform;
	s32 			motorInputIndex;

	// Properties, may be set
	f32 startFollowDistance 	= 2.5f;
	f32 stopFollowDistance 		= 1.5f;

	f32 jumpTimeInterval 		= 15.0f;
	f32 timeToWaitBeforeSit		= 5.0f;

	// State, should not be set
	bool32 	isFollowing;
	f32 	nextJumpTimeTimer;
	f32 	timeWaitedBeforeSitting;
};


FollowerController make_follower_controller(Transform3D * 	transform,
											Transform3D * 	targetTransform,
											s32 			motorInputIndex)
{
	FollowerController controller = {transform, targetTransform, motorInputIndex};
	controller.nextJumpTimeTimer = RandomRange(0, controller.jumpTimeInterval);
	return controller;
}

void update_follower_input(	FollowerController 		& controller,
							Array<CharacterInput> 	& motorInputs,
							f32 elapsedTime)
{
	v3 toTarget 			= controller.targetTransform->position - controller.transform->position;
	toTarget.z 				= 0;
	float distanceToTarget 	= magnitude(toTarget);

	m44 gizmoTransform = make_transform_matrix(	controller.transform->position + up_v3 * controller.transform->scale * 2.0f, 
												controller.transform->rotation,
												0.2f);
	if (distanceToTarget < controller.stopFollowDistance)
	{
		debug::draw_diamond_2d(gizmoTransform, colors::brightGreen, ORIENT_2D_XZ);
	}
	else if (distanceToTarget < controller.startFollowDistance)
	{
		debug::draw_diamond_2d(gizmoTransform, colors::brightYellow, ORIENT_2D_XZ);
	}
	else
	{
		debug::draw_diamond_2d(gizmoTransform, colors::brightRed, ORIENT_2D_XZ);
	}

	v3 inputVector = {};
	if (controller.isFollowing)
	{
		if (distanceToTarget > controller.stopFollowDistance)
		{
			/* Note(Leo): Do not clamp to zero, or we will run out of input prematurely.
			By clamping to a small value, we ensure that while there is distance to go,
			we will always keep  moving.

			Todo(Leo): clamping lower end might be clunky, investigate and maybe fix. */
			inputVector = normalize(toTarget) * math::clamp(distanceToTarget - controller.stopFollowDistance, 0.05f, 1.0f);
		}
		else
		{
			controller.isFollowing = false;
			inputVector = v3{0,0,0};
		}
	}
	else if (controller.isFollowing == false)
	{
		if (distanceToTarget > controller.startFollowDistance)
		{
			inputVector = normalize(toTarget) * math::clamp(distanceToTarget - controller.startFollowDistance, 0.0f, 1.0f);
			controller.isFollowing = true;
		}
		else
		{
			inputVector = v3{0,0,0};
		}
	}

	if (controller.isFollowing)
	{
		gizmoTransform = gizmoTransform * scale_matrix({0.5f, 0.5f, 0.5f});
		debug::draw_diamond_2d(gizmoTransform, colors::brightBlue, ORIENT_2D_XZ);
	}


	bool32 jumpInput = false;
	bool32 crouchInput = false;

	if (controller.isFollowing == false)
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
	
	if (distanceToTarget > controller.startFollowDistance)
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

struct RandomWalkController
{
	Transform3D * transform;
	s32 motorInputIndex;

	v2 targetPosition;
	f32 waitTimer;

	bool isWaiting;
};

void update_random_walker_input(RandomWalkController & controller,
								Array<CharacterInput> & motorInputs,
								f32 elapsedTime)
{
	if (controller.isWaiting)
	{
		controller.waitTimer -= elapsedTime;
		if (controller.waitTimer < 0)
		{
			controller.targetPosition = { RandomRange(-99, 99), RandomRange(-99, 99)};
			controller.isWaiting = false;
		}


		m44 gizmoTransform = make_transform_matrix(	controller.transform->position + up_v3 * controller.transform->scale * 2.0f, 
													controller.transform->rotation,
													controller.waitTimer);
		debug::draw_diamond_2d(gizmoTransform, colors::mutedRed, ORIENT_2D_XZ);
	}
	
	f32 distance = magnitude_v2(*controller.transform->position.xy() - controller.targetPosition);
	if (distance < 1.0f && controller.isWaiting == false)
	{
		controller.waitTimer = 10;
		controller.isWaiting = true;
	}


	v3 input 			= {};
	*input.xy() 		= controller.targetPosition - *controller.transform->position.xy();
	f32 inputMagnitude 	= magnitude(input);
	input 				= input / inputMagnitude;
	inputMagnitude 		= math::clamp(inputMagnitude, 0.0f, 1.0f);
	input 				= input * inputMagnitude;

	motorInputs[controller.motorInputIndex] = {input, false, false};

}