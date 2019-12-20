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
	Update(game::Input * input)
	{
		if (input->zoomIn.IsPressed())
		{
			distance -= distance * relativeZoomSpeed * input->elapsedTime;
			distance = Max(distance, minDistance);
		}
		else if(input->zoomOut.IsPressed())
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

// struct CameraController3rdPerson
// {
// 	// References, these must be set separately
// 	Camera *		camera;
// 	Transform3D * 	target;
	
// 	// State, these change from frame to frame
// 	Vector3 baseOffset 			= {0, 0, 2.0f};
// 	float distance 				= 20.0f;
// 	float orbitDegrees 			= 180.0f;
// 	float tumbleDegrees 		= 0.0f;

// 	// Properties
// 	float rotateSpeed 			= 180.0f;
// 	float minTumble 			= -10.0f;
// 	float maxTumble 			= 85.0f;
// 	float relativeZoomSpeed 	= 1.0f;
// 	float minDistance 			= 5.0f;
// 	float maxDistance 			= 100.0f;

// 	void 
// 	Update(game::Input * input)
// 	{
// 		if (input->zoomIn.IsPressed())
// 		{
// 			distance -= distance * relativeZoomSpeed * input->elapsedTime;
// 			distance = Max(distance, minDistance);
// 		}
// 		else if(input->zoomOut.IsPressed())
// 		{
// 			distance += distance * relativeZoomSpeed * input->elapsedTime;
// 			distance = Min(distance, maxDistance);
// 		}

// 	    orbitDegrees += input->look.x * rotateSpeed * input->elapsedTime;
	    
// 	    tumbleDegrees += input->look.y * rotateSpeed * input->elapsedTime;
// 	    tumbleDegrees = Clamp(tumbleDegrees, minTumble, maxTumble);

// 	    real32 cameraDistance = distance;
// 	    real32 cameraHorizontalDistance = Cosine(DegToRad * tumbleDegrees) * cameraDistance;
// 	    Vector3 localPosition 
// 	    {
// 			Sine(DegToRad * orbitDegrees) * cameraHorizontalDistance,
// 			Cosine(DegToRad * orbitDegrees) * cameraHorizontalDistance,
// 			Sine(DegToRad * tumbleDegrees) * cameraDistance
// 	    };


// 	    /*
// 	    Todo[Camera] (Leo): This is good effect, but its too rough like this,
// 	    make it good later when projections work

// 	    real32 cameraAdvanceAmount = 5;
// 	    Vector3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
// 	    Vector3 targetPosition = target.position + cameraAdvanceVector + cameraOffsetFromTarget;
// 	    */

// 	    Vector3 targetGroundedPosition = target->position;
// 	    targetGroundedPosition.z = 0;
// 	    Vector3 targetPosition = targetGroundedPosition + baseOffset;
	    
// 	    camera->position = targetPosition + localPosition;
// 		camera->LookAt(targetPosition);				
// 	}
// };
