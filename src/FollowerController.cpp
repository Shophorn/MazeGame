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
	controller.nextJumpTimeTimer = random_range(0, controller.jumpTimeInterval);
	return controller;
}

void update_follower_input(	FollowerController 		& controller,
							Array<CharacterInput> 	& motorInputs,
							f32 elapsedTime)
{
	v3 toTarget 			= controller.targetTransform->position - controller.transform->position;
	toTarget.z 				= 0;
	f32 distanceToTarget 	= v3_length(toTarget);

	m44 gizmoTransform = make_transform_matrix(	controller.transform->position + v3_up * controller.transform->scale.z * 2.0f, 
												controller.transform->rotation,
												0.2f);
	if (distanceToTarget < controller.stopFollowDistance)
	{
		FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_bright_green));
	}
	else if (distanceToTarget < controller.startFollowDistance)
	{
		FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_bright_yellow));
	}
	else
	{
		FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_bright_red));
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
			inputVector = normalize_v3(toTarget) * clamp_f32(distanceToTarget - controller.stopFollowDistance, 0.05f, 1.0f);
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
			inputVector = normalize_v3(toTarget) * clamp_f32(distanceToTarget - controller.startFollowDistance, 0.0f, 1.0f);
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
		FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_bright_blue));
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
			controller.targetPosition = { random_range(-99, 99), random_range(-99, 99)};
			controller.isWaiting = false;
		}


		m44 gizmoTransform = make_transform_matrix(	controller.transform->position + v3_up * controller.transform->scale.z * 2.0f, 
													controller.transform->rotation,
													controller.waitTimer);
		FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_muted_red));
	}
	
	f32 distance = v2_length(controller.transform->position.xy - controller.targetPosition);
	if (distance < 1.0f && controller.isWaiting == false)
	{
		controller.waitTimer = 10;
		controller.isWaiting = true;
	}


	v3 input 			= {};
	input.xy	 		= controller.targetPosition - controller.transform->position.xy;
	f32 inputMagnitude 	= v3_length(input);
	input 				= input / inputMagnitude;
	inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
	input 				= input * inputMagnitude;

	motorInputs[controller.motorInputIndex] = {input, false, false};

}