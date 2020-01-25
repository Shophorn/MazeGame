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

internal Camera
make_camera(float fieldOfView, float nearClipPlane, float farClipPlane)
{
	return {
		.fieldOfView 	= fieldOfView,
		.nearClipPlane 	= nearClipPlane,
		.farClipPlane 	= farClipPlane,
	};
}

Matrix44
get_perspective_projection(Camera const * cam)
{
	/*
	Todo(Leo): Do studies, this really affects many things
	Study: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix

	Yay, works for now
	*/
	float canvasSize = Tan(ToRadians(cam->fieldOfView / 2.0f)) * cam->nearClipPlane;

	// Note(Leo): Vulkan NDC goes left-top = (-1, -1) to right-bottom = (1, 1)
	float bottom = canvasSize / 2;
	float top = -bottom;

	float right = bottom * cam->aspectRatio;
	float left = -right;

    float
    	b = bottom,
    	t = top,
    	r = right,
    	l = left,

    	n = cam->nearClipPlane,
    	f = cam->farClipPlane;


    /* Study(Leo): why does this seem to use y-up and z-forward,
    when actual world coordinates are z-up and y-forward */
	Matrix44 result = {
		2*n / (r - l), 		0, 					0, 							0,
		0, 					-2 * n / (b - t), 	0, 							0,
		(r + l)/(r - l), 	(b + t)/(b - t),	-((f + n)/(f -n)),			-1,
		0, 					0, 					-(2 * f * n / (f - n)), 	0
	};

	return result;	
}

Matrix44
get_view_projection(Camera const * cam)
{
	// Study: https://www.3dgep.com/understanding-the-view-matrix/

	vector3 zAxis 	= -cam->forward;
	vector3 xAxis 	= vector::normalize(vector::cross(world::up, zAxis));
	vector3 yAxis 	= vector::cross (zAxis, xAxis);

	Matrix44 orientation = {
		xAxis[0], yAxis[0], zAxis[0], 0,
		xAxis[1], yAxis[1], zAxis[1], 0,
		xAxis[2], yAxis[2], zAxis[2], 0,
		0, 0, 0, 1
	};
	
	/* Todo(Leo): Translation can be done inline with matrix initialization
	instead of as separate function call */
	Matrix44 translation = Matrix44::Translate(-cam->position);
	Matrix44 result = orientation * translation;

	return result;	
}

Matrix44 
get_rotation_matrix(Camera * camera)
{
	vector3 zAxis 	= -camera->forward;
	vector3 xAxis 	= vector::normalize(vector::cross(world::up, zAxis));
	vector3 yAxis 	= vector::cross (zAxis, xAxis);

	Matrix44 orientation = {
		xAxis[0], yAxis[0], zAxis[0], 0,
		xAxis[1], yAxis[1], zAxis[1], 0,
		xAxis[2], yAxis[2], zAxis[2], 0,
		0, 0, 0, 1
	};

	return orientation;
}