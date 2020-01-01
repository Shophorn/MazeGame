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
update_camera_system(game::RenderInfo * renderer, game::PlatformInfo * platform, game::Input * input, Camera * camera)
{
	// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
    camera->aspectRatio = (real32)platform->windowWidth / (real32)platform->windowHeight;

	// Ccamera
    renderer->set_camera(	camera->ViewProjection(),
							camera->PerspectiveProjection());
}



struct CameraController3rdPerson
{
	// References, these must be set separately
	Camera *				camera;
	Handle<Transform3D> 	target;
	
	// State, these change from frame to frame
	Vector3 baseOffset 			= {0, 0, 2.0f};
	float distance 				= 20.0f;
	float orbitDegrees 			= 180.0f;
	float tumbleDegrees 		= 0.0f;

	// Properties
	float rotateSpeed 			= 180.0f;
	float minTumble 			= -85.0f;
	float maxTumble 			= 85.0f;
	float relativeZoomSpeed 	= 1.0f;
	float minDistance 			= 5.0f;
	float maxDistance 			= 100.0f;

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

	    orbitDegrees += input->look.x * rotateSpeed * input->elapsedTime;
	    
	    tumbleDegrees += input->look.y * rotateSpeed * input->elapsedTime;
	    tumbleDegrees = Clamp(tumbleDegrees, minTumble, maxTumble);

	    real32 cameraDistance = distance;
	    real32 cameraHorizontalDistance = Cosine(DegToRad * tumbleDegrees) * cameraDistance;
	    Vector3 localPosition 
	    {
			Sine(DegToRad * orbitDegrees) * cameraHorizontalDistance,
			Cosine(DegToRad * orbitDegrees) * cameraHorizontalDistance,
			Sine(DegToRad * tumbleDegrees) * cameraDistance
	    };

	    /*
	    Todo[Camera] (Leo): This is good effect, but its too rough like this,
	    make it good later when projections work

	    real32 cameraAdvanceAmount = 5;
	    Vector3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
	    Vector3 targetPosition = target.position + cameraAdvanceVector + cameraOffsetFromTarget;
	    */

	    Vector3 targetGroundedPosition = target->position;
	    targetGroundedPosition.z = 0;
	    Vector3 targetPosition = targetGroundedPosition + baseOffset;
	    
	    camera->position = targetPosition + localPosition;
		camera->LookAt(targetPosition);				
	}
};

// Todo(Leo): Do I want this really?
internal void
update(CameraController3rdPerson * controller, game::Input * input)
{
	controller->update(input);
}