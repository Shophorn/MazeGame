struct CameraControllerSideScroller
{
	// References
	Camera * 		camera;
	Transform3D *	target;

	// State
	v3 baseOffset 	= {0, 0, 1};
	f32 distance 		= 20.0f;

	// Properties
	f32 minDistance 			= 5.0f;
	f32 maxDistance 			= 100.0f;
	f32 relativeZoomSpeed 	= 1.0f;

	void
	update(game::Input * input)
	{
		if (is_pressed(input->zoomIn))
		{
			distance -= distance * relativeZoomSpeed * input->elapsedTime;
			distance = math::max(distance, minDistance);
		}
		else if(is_pressed(input->zoomOut))
		{
			distance += distance * relativeZoomSpeed * input->elapsedTime;
			distance = math::min(distance, maxDistance);
		}

		v3 direction = forward_v3;
		camera->direction = direction;
		camera->position = 	baseOffset
							+ target->position
							- (direction * distance); 
	}
};

internal void
update_camera_system(	Camera * camera,
						game::Input * input,
						platform::Graphics * graphics,
						platform::Window * window)
{
	/* Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	This probably costs us less than checking if it has :D */
    camera->aspectRatio = (f32)platformApi->get_window_width(window) / (f32)platformApi->get_window_height(window);
    platformApi->update_camera(graphics, camera);
}

struct PlayerCameraController
{
	// State, these change from frame to frame
	v3 position;
	v3 direction;
	v3 lastTrackedPosition;

	f32 distance 			= 20.0f;
	f32 orbitDegrees 		= 180.0f;
	f32 tumbleDegrees 		= 0.0f;

	// Properties
	v3 baseOffset 			= {0, 0, 3.0f};
	f32 rotateSpeed 		= 180.0f;
	f32 minTumble 			= -85.0f;
	f32 maxTumble 			= 85.0f;
	f32 relativeZoomSpeed 	= 1.0f;
	f32 minDistance 		= 2.0f;
	f32 maxDistance 		= 100.0f;
};

internal void
update_camera_controller(PlayerCameraController * controller, v3 targetPosition, game::Input * input)
{
	if (is_pressed(input->zoomIn))
	{
		controller->distance -= controller->distance * controller->relativeZoomSpeed * input->elapsedTime;
		controller->distance = math::max(controller->distance, controller->minDistance);
	}
	else if(is_pressed(input->zoomOut))
	{
		controller->distance += controller->distance * controller->relativeZoomSpeed * input->elapsedTime;
		controller->distance = math::min(controller->distance, controller->maxDistance);
	}

    controller->orbitDegrees += input->look.x * controller->rotateSpeed * input->elapsedTime;
    
    controller->tumbleDegrees += input->look.y * controller->rotateSpeed * input->elapsedTime;
    controller->tumbleDegrees = math::clamp(controller->tumbleDegrees, controller->minTumble, controller->maxTumble);

    f32 cameraDistance = controller->distance;
    f32 cameraHorizontalDistance = cosine(to_radians(controller->tumbleDegrees)) * cameraDistance;
    v3 localPosition =
    {
		sine(to_radians(controller->orbitDegrees)) * cameraHorizontalDistance,
		cosine(to_radians(controller->orbitDegrees)) * cameraHorizontalDistance,
		sine(to_radians(controller->tumbleDegrees)) * cameraDistance
    };

    /*
    Todo[Camera] (Leo): This is good effect, but its too rough like this,
    make it good later when projections work

    f32 cameraAdvanceAmount = 5;
    v3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
    v3 targetPosition = target.position + cameraAdvanceVector + cameraOffsetFromTarget;
    */

    f32 trackSpeed = 10 * input->elapsedTime;
    f32 z = interpolate(controller->lastTrackedPosition.z, targetPosition.z, trackSpeed);


    // Todo(Leo): Maybe track faster on xy plane, instead of teleporting
    // v3 trackedPosition = vector::interpolate(	controller->lastTrackedPosition,
    // 												targetPosition,
    // 												trackSpeed);
    v3 trackedPosition =
    {
    	targetPosition.x,
    	targetPosition.y,
    	// controller->target->position.z
    	z
    };


    controller->lastTrackedPosition = trackedPosition;

	controller->position = trackedPosition + controller->baseOffset + localPosition;
	controller->direction = -normalize_v3(localPosition);

	Assert(math::distance(square_magnitude_v3(controller->direction), 1.0f) < 0.00001f);
}

struct FreeCameraController
{
	v3 position;
	v3 direction;

	f32 moveSpeed = 30;
	f32 zMoveSpeed = 15;
	f32 rotateSpeed = 0.5f * tau;
	f32 panAngle;
	f32 tiltAngle;
	f32 maxTilt = 0.2f * tau;
};

internal void update_free_camera(FreeCameraController & controller, game::Input const & input)
{
	// Note(Leo): positive rotation is to left, which is opposite of joystick
	controller.panAngle += -1 * input.look.x * controller.rotateSpeed * input.elapsedTime;

	controller.tiltAngle += -1 * input.look.y * controller.rotateSpeed * input.elapsedTime;
	controller.tiltAngle = math::clamp(controller.tiltAngle, -controller.maxTilt, controller.maxTilt);

	quaternion pan = axis_angle_quaternion(up_v3, controller.panAngle);
	m44 panMatrix = rotation_matrix(pan);

	// Todo(Leo): somewhy this points to opposite of right
	v3 right = multiply_direction(panMatrix, right_v3);
	v3 forward = multiply_direction(panMatrix, forward_v3);

	f32 moveStep 		= controller.moveSpeed * input.elapsedTime;
	v3 rightMovement 	= right * input.move.x * moveStep;
	v3 forwardMovement 	= forward * input.move.y * moveStep;

	f32 zInput = is_pressed(input.zoomOut) - is_pressed(input.zoomIn);
	v3 upMovement = up_v3 * zInput * moveStep;

	quaternion tilt = axis_angle_quaternion(right, controller.tiltAngle);
	m44 rotation 	= rotation_matrix(pan * tilt);
	v3 direction 	= multiply_direction(rotation, forward_v3);
	
	controller.position += rightMovement + forwardMovement + upMovement;
	controller.direction = direction;
}