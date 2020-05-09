struct CameraControllerSideScroller
{
	// References
	Camera * 		camera;
	Transform3D *	target;

	// State
	v3 baseOffset 	= {0, 0, 1};
	float distance 		= 20.0f;

	// Properties
	float minDistance 			= 5.0f;
	float maxDistance 			= 100.0f;
	float relativeZoomSpeed 	= 1.0f;

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

		v3 direction = world::forward;
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
    camera->aspectRatio = (float)platformApi->get_window_width(window) / (float)platformApi->get_window_height(window);
    platformApi->update_camera(graphics, camera);
}

struct CameraController3rdPerson
{
	// References, these must be set separately
	Camera *				camera;
	Transform3D * 			target;
	
	// This is also property but it is calculated when creating the camera
	v3 lastTrackedPosition;
	
	// State, these change from frame to frame
	float distance 				= 20.0f;
	float orbitDegrees 			= 180.0f;
	float tumbleDegrees 		= 0.0f;


	// Properties
	v3 baseOffset 			= {0, 0, 3.0f};
	float rotateSpeed 			= 180.0f;
	float minTumble 			= -85.0f;
	float maxTumble 			= 85.0f;
	float relativeZoomSpeed 	= 1.0f;
	float minDistance 			= 2.0f;
	float maxDistance 			= 100.0f;

	s32 zoomMode;
};

internal CameraController3rdPerson
make_camera_controller_3rd_person(Camera * camera, Transform3D * target)
{
	return {
		.camera 			= camera,
		.target 			= target,
		.lastTrackedPosition = target->position
	};
}

internal void
update_camera_controller(CameraController3rdPerson * controller, v3 * cameraPosition, game::Input * input)
{
	if (is_clicked(input->Y))
	{
		controller->zoomMode += 1;
		controller->zoomMode %= 2;
	}

	if (controller->zoomMode == 0)
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
	}
	else
	{
		if (is_pressed(input->zoomIn))
		{
			controller->baseOffset.z -= 1.0f * input->elapsedTime;
		}
		else if (is_pressed(input->zoomOut))
		{
			controller->baseOffset.z += 1.0f * input->elapsedTime;
		}
	}

    controller->orbitDegrees += input->look.x * controller->rotateSpeed * input->elapsedTime;
    
    controller->tumbleDegrees += input->look.y * controller->rotateSpeed * input->elapsedTime;
    controller->tumbleDegrees = math::clamp(controller->tumbleDegrees, controller->minTumble, controller->maxTumble);

    float cameraDistance = controller->distance;
    float cameraHorizontalDistance = cosine(to_radians(controller->tumbleDegrees)) * cameraDistance;
    v3 localPosition =
    {
		sine(to_radians(controller->orbitDegrees)) * cameraHorizontalDistance,
		cosine(to_radians(controller->orbitDegrees)) * cameraHorizontalDistance,
		sine(to_radians(controller->tumbleDegrees)) * cameraDistance
    };

    /*
    Todo[Camera] (Leo): This is good effect, but its too rough like this,
    make it good later when projections work

    float cameraAdvanceAmount = 5;
    v3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
    v3 targetPosition = target.position + cameraAdvanceVector + cameraOffsetFromTarget;
    */

    float trackSpeed = 10 * input->elapsedTime;
    float z = interpolate(controller->lastTrackedPosition.z, controller->target->position.z, trackSpeed);


    // Todo(Leo): Maybe track faster on xy plane, instead of teleporting
    // v3 trackedPosition = vector::interpolate(	controller->lastTrackedPosition,
    // 												controller->target->position,
    // 												trackSpeed);
    v3 trackedPosition =
    {
    	controller->target->position.x,
    	controller->target->position.y,
    	// controller->target->position.z
    	z
    };


    controller->lastTrackedPosition = trackedPosition;

    v3 targetPosition = trackedPosition + controller->baseOffset;
    
    // controller->camera->position = targetPosition + localPosition;
    *cameraPosition = targetPosition + localPosition;
	controller->camera->direction = -1.0f * localPosition.normalized();

	Assert(math::distance(controller->camera->direction.square_magnitude(), 1.0f) < 0.00001f);
}