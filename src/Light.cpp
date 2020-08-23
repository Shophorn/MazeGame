/*=====================================
Leo Tamminen
shophorn @ internet

This is light in all its anticlimaxiness.
=====================================*/
struct Light
{
	v3 direction;
	v3 color;

	f32 shadowDistance;
	f32 shadowTransitionDistance;

	f32 skyColorSelection;
	
	f32 hdrExposure;
	f32 hdrContrast;

	v4 skyBottomColor;
	v4 skyTopColor;

	v4 horizonHaloColorAndFalloff;
	v4 sunHaloColorAndFalloff;
};


internal m44
get_light_view_projection(Light const * directionalLight, Camera const * camera)
{
	// compute camera view frustrum corners in world space
	// transform corners to light space
	// compute bounding volume of corners
	// 	-> width, depth and height makes light projection
	// 	-> back plane and directionalLight direction makes light view

/*
	struct Camera
	{
		v3 position;
		// Todo(Leo): change to quaternion, maybe
		v3 forward = world::forward;

		f32 fieldOfView;
		f32 nearClipPlane;
		f32 farClipPlane;
		f32 aspectRatio;
	};
*/
	f32 shadowDistance = 100;

	v3 cameraRight 		= get_right(camera);
	v3 cameraForward 	= get_forward(camera);
	v3 cameraUp 		= get_up(camera);

	v3 right 			= cameraRight * Tan(camera->verticalFieldOfView / 2.0f) * camera->aspectRatio;
	v3 forward 			= cameraForward;
	v3 up 				= cameraUp * Tan(camera->verticalFieldOfView / 2.0f);

	v3 nearTopLeft 		= camera->position + (forward + up - right) * camera->nearClipPlane;
	v3 nearTopRight 	= camera->position + (forward + up + right) * camera->nearClipPlane;
	v3 nearBottomLeft 	= camera->position + (forward - up - right) * camera->nearClipPlane;
	v3 nearBottomRight 	= camera->position + (forward - up + right) * camera->nearClipPlane;

	v3 farTopLeft 		= camera->position + (forward + up - right) * shadowDistance;
	v3 farTopRight 		= camera->position + (forward + up + right) * shadowDistance;
	v3 farBottomLeft 	= camera->position + (forward - up - right) * shadowDistance;
	v3 farBottomRight 	= camera->position + (forward - up + right) * shadowDistance;


	v3 lightPosition 	= (nearTopLeft + nearTopRight + nearBottomLeft + nearBottomRight + farTopLeft + farTopRight + farBottomLeft + farBottomRight) / 8.0f;

	Camera lightCamera 		= {};
	lightCamera.position 	= lightPosition - directionalLight->direction * 100.0f;
	lightCamera.direction 	= directionalLight->direction;

	m44 lightView 			= camera_view_matrix(lightCamera.position, lightCamera.direction);

	m44 inverseLightView = 
	{
		lightView[0].x, lightView[1].x, lightView[2].x, 0,
		lightView[0].y, lightView[1].y, lightView[2].y, 0,
		lightView[0].z, lightView[1].z, lightView[2].z, 0,
		-lightView[3].x, -lightView[3].y, -lightView[3].z, 1
	};

	v3 points [] =
	{
		multiply_point(inverseLightView, nearTopLeft), 	
		multiply_point(inverseLightView, nearTopRight), 
		multiply_point(inverseLightView, nearBottomLeft), 
		multiply_point(inverseLightView, nearBottomRight),
		multiply_point(inverseLightView, farTopLeft), 	
		multiply_point(inverseLightView, farTopRight), 	
		multiply_point(inverseLightView, farBottomLeft), 
		multiply_point(inverseLightView, farBottomRight), 
	};

	f32 xMin = highest_f32;
	f32 yMin = highest_f32;
	f32 zMin = highest_f32;

	f32 xMax = lowest_f32;
	f32 yMax = lowest_f32;
	f32 zMax = lowest_f32;

	for (v3 point : points)
	{
		xMin = min_f32(xMin, point.x);
		yMin = min_f32(yMin, point.y);
		zMin = min_f32(zMin, point.z);

		xMax = max_f32(xMax, point.x);
		yMax = max_f32(yMax, point.y);
		zMax = max_f32(zMax, point.z);
	} 
	// f32 

	// m44 lightRotation = lightView;
	// lightRotation[3] = {0, 0, 0, 1};

	// // m44 inverseLightRotation = matrix::transpose(lightRotation);
	// // // Study(Leo): Rotation matrices are "orthogonal", so they can be inverted by taking transpose

	// // m44 inverseLightTranslation = make_translation_matrix(-lightPosition);

	// // // right: TR
	// // // inverse: RiTi

	// // m44 inverseLightTransform = matrix::multiply(inverseLightRotation,inverseLightTranslation);

	// // auto transform_to_light_space = [&](v3 worldPosition)
	// // {	
	// // 	worldPosition = inverseLightTransform * worldPosition;
	// // 	return worldPosition;
	// // };

	// // nearTopLeft		= transform_to_light_space(nearTopLeft);
	// // nearTopRight	= transform_to_light_space(nearTopRight);
	// // nearBottomLeft	= transform_to_light_space(nearBottomLeft);
	// // nearBottomRight = transform_to_light_space(nearBottomRight);
	// // farTopLeft		= transform_to_light_space(farTopLeft);
	// // farTopRight		= transform_to_light_space(farTopRight);
	// // farBottomLeft	= transform_to_light_space(farBottomLeft);
	// // farBottomRight	= transform_to_light_space(farBottomRight);



	// f32 shadowBoxWidth = xMax - xMin;//100;
	// f32 shadowBoxDepth = yMax - yMin; //-100;
	// f32 shadowBoxHeight = zMax - zMin; //-500;


	f32 shadowBoxWidth = 100;
	f32 shadowBoxDepth = -100;
	f32 shadowBoxHeight = -500;


	// logDebug(0) << "ShadowBox = " << shadowBoxWidth << "*" << shadowBoxDepth << "*" << shadowBoxHeight; 

	m44 lightProjection =
	{
		2.0f / shadowBoxWidth, 0, 0, 0,
		0, 2.0f / shadowBoxDepth, 0, 0,
		0, 0, 2.0f / shadowBoxHeight, 0,
		0, 0, 0, 1
	};

	return lightProjection * lightView;
}