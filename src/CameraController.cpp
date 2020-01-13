struct CameraControllerSideScroller
{
	// References
	Camera * 		camera;
	Handle<Transform3D>	target;

	// State
	Vector3 baseOffset = {0, 0, 1};
	float distance = 20.0f;

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
			distance = Max(distance, minDistance);
		}
		else if(is_pressed(input->zoomOut))
		{
			distance += distance * relativeZoomSpeed * input->elapsedTime;
			distance = Min(distance, maxDistance);
		}

		Vector3 direction = World::Forward;
		camera->forward = direction;
		camera->position = 	baseOffset
							+ target->position
							- (direction * distance); 
	}
};

internal void
update_camera_system(game::RenderInfo * renderer, game::PlatformInfo * platform, game::Input * input, Camera * camera, platform::GraphicsContext * graphics)
{
	/* Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	This probably costs us less than checking if it has :D */
    camera->aspectRatio = (float)platform->windowWidth / (float)platform->windowHeight;
    renderer->update_camera(graphics,
    						camera->ViewProjection(),
							camera->PerspectiveProjection());
}

struct CameraController3rdPerson
{
	// References, these must be set separately
	Camera *				camera;
	Handle<Transform3D> 	target;
	
	// This is also property but it is calculated when creating the camera
	Vector3 lastTrackedPosition;
	
	// State, these change from frame to frame
	float distance 				= 20.0f;
	float orbitDegrees 			= 180.0f;
	float tumbleDegrees 		= 0.0f;


	// Properties
	Vector3 baseOffset 			= {0, 0, 3.0f};
	float rotateSpeed 			= 180.0f;
	float minTumble 			= -85.0f;
	float maxTumble 			= 85.0f;
	float relativeZoomSpeed 	= 1.0f;
	float minDistance 			= 5.0f;
	float maxDistance 			= 100.0f;
};

internal CameraController3rdPerson
make_camera_controller(Camera * camera, Handle<Transform3D> target)
{
	return {
		.camera 			= camera,
		.target 			= target,
		.lastTrackedPosition = target->position
	};
}

// Todo(Leo): Do I want this really?
internal void
update_camera_controller(CameraController3rdPerson * controller, game::Input * input)
{
	if (is_pressed(input->zoomIn))
	{
		controller->distance -= controller->distance * controller->relativeZoomSpeed * input->elapsedTime;
		controller->distance = Max(controller->distance, controller->minDistance);
	}
	else if(is_pressed(input->zoomOut))
	{
		controller->distance += controller->distance * controller->relativeZoomSpeed * input->elapsedTime;
		controller->distance = Min(controller->distance, controller->maxDistance);
	}

    controller->orbitDegrees += input->look.x * controller->rotateSpeed * input->elapsedTime;
    
    controller->tumbleDegrees += input->look.y * controller->rotateSpeed * input->elapsedTime;
    controller->tumbleDegrees = clamp(controller->tumbleDegrees, controller->minTumble, controller->maxTumble);

    float cameraDistance = controller->distance;
    float cameraHorizontalDistance = Cosine(DegToRad * controller->tumbleDegrees) * cameraDistance;
    Vector3 localPosition =
    {
		Sine(DegToRad * controller->orbitDegrees) * cameraHorizontalDistance,
		Cosine(DegToRad * controller->orbitDegrees) * cameraHorizontalDistance,
		Sine(DegToRad * controller->tumbleDegrees) * cameraDistance
    };

    /*
    Todo[Camera] (Leo): This is good effect, but its too rough like this,
    make it good later when projections work

    float cameraAdvanceAmount = 5;
    Vector3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
    Vector3 targetPosition = target.position + cameraAdvanceVector + cameraOffsetFromTarget;
    */

    float trackSpeed = 10 * input->elapsedTime;
    float z = interpolate(controller->lastTrackedPosition.z, controller->target->position.z, trackSpeed);


    // Vector3 trackedPosition = vector::interpolate(	controller->lastTrackedPosition,
    // 												controller->target->position,
    // 												trackSpeed);
    Vector3 trackedPosition =
    {
    	controller->target->position.x,
    	controller->target->position.y,
    	z
    };
    controller->lastTrackedPosition = trackedPosition;

    Vector3 targetPosition = trackedPosition + controller->baseOffset;
    
    controller->camera->position = targetPosition + localPosition;
	controller->camera->LookAt(targetPosition);				
}