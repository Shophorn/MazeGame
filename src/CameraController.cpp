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
	update(PlatformInput const & input, f32 elapsedTime)
	{
		if (is_pressed(input.zoomIn))
		{
			distance -= distance * relativeZoomSpeed * elapsedTime;
			distance = max_f32(distance, minDistance);
		}
		else if(is_pressed(input.zoomOut))
		{
			distance += distance * relativeZoomSpeed * elapsedTime;
			distance = min_f32(distance, maxDistance);
		}

		v3 direction = v3_forward;
		camera->direction = direction;
		camera->position = 	baseOffset
							+ target->position
							- (direction * distance); 
	}
};

internal void
update_camera_system(	Camera * camera,
						PlatformGraphics * graphics,
						PlatformWindow * window)
{
	/* Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	This probably costs us less than checking if it has :D */
    camera->aspectRatio = (f32)platform_window_get_width(window) / (f32)platform_window_get_height(window);
    graphics_drawing_update_camera(graphics, camera);
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
update_camera_controller(PlayerCameraController * controller, v3 targetPosition, PlatformInput const & input, f32 elapsedTime)
{
	if (is_pressed(input.zoomIn))
	{
		controller->distance -= controller->distance * controller->relativeZoomSpeed * elapsedTime;
		controller->distance = max_f32(controller->distance, controller->minDistance);
	}
	else if(is_pressed(input.zoomOut))
	{
		controller->distance += controller->distance * controller->relativeZoomSpeed * elapsedTime;
		controller->distance = min_f32(controller->distance, controller->maxDistance);
	}

    controller->orbitDegrees += input.look.x * controller->rotateSpeed * elapsedTime;
    
    controller->tumbleDegrees += input.look.y * controller->rotateSpeed * elapsedTime;
    controller->tumbleDegrees = clamp_f32(controller->tumbleDegrees, controller->minTumble, controller->maxTumble);

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

    f32 trackSpeed = 10 * elapsedTime;
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

	Assert(abs_f32(square_magnitude_v3(controller->direction) - 1.0f) < 0.00001f);
}

struct FreeCameraController
{
	v3 position;
	v3 direction;
	f32 panAngle;
	f32 tiltAngle;
};

internal m44 update_free_camera(FreeCameraController & controller, PlatformInput const & input, f32 elapsedTime)
{
	f32 lowMoveSpeed 	= 10;
	f32 highMoveSpeed	= 100;
	f32 zMoveSpeed 		= 25;
	f32 rotateSpeed 	= π;
	f32 maxTilt 		= 0.45f * π;

	// Note(Leo): positive rotation is to left, which is opposite of joystick
	controller.panAngle += -1 * input.look.x * rotateSpeed * elapsedTime;

	controller.tiltAngle += -1 * input.look.y * rotateSpeed * elapsedTime;
	controller.tiltAngle = clamp_f32(controller.tiltAngle, -maxTilt, maxTilt);

	quaternion pan 	= axis_angle_quaternion(v3_up, controller.panAngle);
	m44 panMatrix 	= rotation_matrix(pan);

	// Todo(Leo): somewhy this points to opposite of right
	// Note(Leo): It was maybe that camera was just upside down, but corrected only in vertical direction
	v3 right 	= multiply_direction(panMatrix, v3_right);
	v3 forward 	= multiply_direction(panMatrix, v3_forward);

	constexpr f32 minHeight = 10;
	constexpr f32 maxHeight = 50;
	f32 heightValue = clamp_f32(controller.position.z, minHeight, maxHeight) / maxHeight;
	f32 moveSpeed 	= interpolate(lowMoveSpeed, highMoveSpeed, heightValue);

	f32 moveStep 		= moveSpeed * elapsedTime;
	v3 rightMovement 	= right * input.move.x * moveStep;
	v3 forwardMovement 	= forward * input.move.y * moveStep;

	f32 zInput 		= is_pressed(input.zoomOut) - is_pressed(input.zoomIn);
	v3 upMovement 	= v3_up * zInput * zMoveSpeed * elapsedTime;

	quaternion tilt = axis_angle_quaternion(right, controller.tiltAngle);
	m44 rotation 	= rotation_matrix(pan * tilt);
	v3 direction 	= multiply_direction(rotation, v3_forward);
	
	controller.position += rightMovement + forwardMovement + upMovement;
	controller.direction = direction;

	return rotation;
}

struct MouseCameraController
{
	v3 targetPosition 	= {0,0,0};
	v3 position 		= {0,0,0};
	v3 direction 		= {0,1,0};
	f32 distance 		= 0;
	v2 lastMousePosition;
};

internal void update_mouse_camera(MouseCameraController & controller, PlatformInput const & input, f32 elapsedTime)
{

	constexpr f32 lowMoveSpeed 		= 10;
	constexpr f32 highMoveSpeed		= 100;
	constexpr f32 zMoveSpeed 		= 25;
	constexpr f32 rotateSpeed 		= 0.3;
	constexpr f32 maxTilt 			= 0.45f * π;
	constexpr f32 scrollMoveSpeed 	= -10; // Note(Leo): Negative because scroll up should move us closer

	v2 mouseInput 					= input.mousePosition - controller.lastMousePosition;
	controller.lastMousePosition 	= input.mousePosition;

	if (is_pressed(input.mouse0) == false)
	{
		mouseInput = {};
	}

	// Note(Leo): positive rotation is to left, which is opposite of joystick
	f32 panAngle 	= -1 * mouseInput.x * rotateSpeed * elapsedTime;
	quaternion pan 	= axis_angle_quaternion(v3_up, panAngle);

	v3 direction 	= rotate_v3(pan, controller.direction);
	v3 right 		= normalize_v3(cross_v3(direction, v3_up));

	f32 tiltAngle 	= -1 * mouseInput.y * rotateSpeed * elapsedTime;
	tiltAngle 		= clamp_f32(tiltAngle, -maxTilt, maxTilt);
	quaternion tilt = axis_angle_quaternion(right, tiltAngle);

	direction 		= rotate_v3(tilt, direction);

	constexpr f32 minHeight = 10;
	constexpr f32 maxHeight = 50;
	f32 heightValue = clamp_f32(controller.targetPosition.z, minHeight, maxHeight) / maxHeight;
	f32 moveSpeed 	= interpolate(lowMoveSpeed, highMoveSpeed, heightValue);

	f32 moveStep 		= moveSpeed * elapsedTime;

	v3 rightMovement 	= right * input.move.x * moveStep;
	v3 forwardMovement 	= direction * input.move.y * moveStep;

	controller.targetPosition += rightMovement + forwardMovement;
	controller.direction = direction;

	f32 scrollMovement 	= input.mouseZoom * scrollMoveSpeed;
	controller.distance += scrollMovement;
	controller.distance = clamp_f32(controller.distance, 0, 10000);

	controller.position = controller.targetPosition - controller.direction * controller.distance;
}