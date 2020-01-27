/*=====================================
Leo Tamminen
shophorn @ internet

This is light in all its anticlimaxiness.
=====================================*/
struct Light
{
	vector3 direction;
	float3 color;
};


internal Matrix44
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
		vector3 position;
		// Todo(Leo): change to quaternion, maybe
		vector3 forward = world::forward;

		float fieldOfView;
		float nearClipPlane;
		float farClipPlane;
		float aspectRatio;
	};
*/
	float shadowDistance = 100;

	vector3 cameraRight 	= get_right(camera);
	vector3 cameraForward 	= get_forward(camera);
	vector3 cameraUp 		= get_up(camera);

	vector3 right = cameraRight * Tan(camera->verticalFieldOfView / 2.0f) * camera->aspectRatio;
	vector3 forward = cameraForward;
	vector3 up = cameraUp * Tan(camera->verticalFieldOfView / 2.0f);

	vector3 nearTopLeft 	= camera->position + (forward + up - right) * camera->nearClipPlane;
	vector3 nearTopRight 	= camera->position + (forward + up + right) * camera->nearClipPlane;
	vector3 nearBottomLeft 	= camera->position + (forward - up - right) * camera->nearClipPlane;
	vector3 nearBottomRight = camera->position + (forward - up + right) * camera->nearClipPlane;

	vector3 farTopLeft 		= camera->position + (forward + up - right) * shadowDistance;
	vector3 farTopRight 	= camera->position + (forward + up + right) * shadowDistance;
	vector3 farBottomLeft 	= camera->position + (forward - up - right) * shadowDistance;
	vector3 farBottomRight 	= camera->position + (forward - up + right) * shadowDistance;


	vector3 lightPosition = (nearTopLeft + nearTopRight + nearBottomLeft + nearBottomRight + farTopLeft + farTopRight + farBottomLeft + farBottomRight) / 8.0f;

	Camera lightCamera = {};
	lightCamera.position = lightPosition - directionalLight->direction * 100.0f;
	lightCamera.direction = directionalLight->direction;

	Matrix44 lightView = get_view_transform(&lightCamera);

	// Matrix44 lightRotation = lightView;
	// lightRotation[3] = {0, 0, 0, 1};

	// // Matrix44 inverseLightRotation = matrix::transpose(lightRotation);
	// // // Study(Leo): Rotation matrices are "orthogonal", so they can be inverted by taking transpose

	// // Matrix44 inverseLightTranslation = make_translation_matrix(-lightPosition);

	// // // right: TR
	// // // inverse: RiTi

	// // Matrix44 inverseLightTransform = matrix::multiply(inverseLightRotation,inverseLightTranslation);

	// // auto transform_to_light_space = [&](vector3 worldPosition)
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



	float shadowBoxWidth = 100;
	float shadowBoxDepth = -100;
	float shadowBoxHeight = -500;

	Matrix44 lightProjection =
	{
		2.0f / shadowBoxWidth, 0, 0, 0,
		0, 2.0f / shadowBoxDepth, 0, 0,
		0, 0, 2.0f / shadowBoxHeight, 0,
		0, 0, 0, 1
	};

	return matrix::multiply(lightProjection, lightView);
}