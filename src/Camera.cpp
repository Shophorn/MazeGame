struct Camera
{
	vector3 position;
	// Todo(Leo): change to quaternion, maybe
	vector3 direction = world::forward;

	float verticalFieldOfView;
	float nearClipPlane;
	float farClipPlane;
	float aspectRatio;
};

internal vector3 get_forward(Camera const*);
internal vector3 get_up(Camera const *);
internal vector3 get_right(Camera const *);

internal Camera
make_camera(float fieldOfView, float nearClipPlane, float farClipPlane)
{
	return {
		.verticalFieldOfView 	= fieldOfView,
		.nearClipPlane 			= nearClipPlane,
		.farClipPlane 			= farClipPlane,
	};
}

Matrix44
get_projection_transform(Camera const * camera)
{
	/*
	Todo(Leo): Do studies, this really affects many things
	Study: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix
	*/

	float canvasSize = Tan(ToRadians(camera->verticalFieldOfView / 2.0f)) * camera->nearClipPlane;

	// Note(Leo): Vulkan NDC goes left-top = (-1, -1) to right-bottom = (1, 1)
	float bottom = canvasSize;
	float top = -bottom;

	float right = bottom * camera->aspectRatio;
	float left = -right;

    float
    	b = bottom,
    	t = top,
    	r = right,
    	l = left,

    	n = camera->nearClipPlane,
    	f = camera->farClipPlane;



    /* Study(Leo): why does this seem to use y-up and z-forward,
    when actual world coordinates are z-up and y-forward */
    // Todo(Leo): change axises here and in get_view_transform()
	Matrix44 result = {
		2*n / (r - l), 		0, 					0, 							0,
		0, 					-2 * n / (b - t), 	0, 							0,
		(r + l)/(r - l), 	(b + t)/(b - t),	-((f + n)/(f -n)),			-1,
		0, 					0, 					-(2 * f * n / (f - n)), 	0
	};

	return result;	
}

Matrix44
get_view_transform(Camera const * camera)
{
	using namespace vector;

	// Study: https://www.3dgep.com/understanding-the-view-matrix/
	// Todo(Leo): try to undeerstand why this is negative
	vector3 yAxis 	= -camera->direction;
	vector3 xAxis 	= normalize(cross(world::up, yAxis));

	/* Note(Leo): this is not normalized because both components are unit length,
	AND they are orthogonal so they produce a unit length vector anyway.
	They are surely orthogonal because xAxis was a cross product from yAxis(with world::up) */
	vector3 zAxis 	= cross (yAxis, xAxis);

	Matrix44 orientation = {
		xAxis.x, zAxis.x, yAxis.x, 0,
		xAxis.y, zAxis.y, yAxis.y, 0,
		xAxis.z, zAxis.z, yAxis.z, 0,
		0, 0, 0, 1
	};
	
	/* Todo(Leo): Translation can be done inline with matrix initialization
	instead of as separate function call */
	Matrix44 translation = make_translation_matrix(-camera->position);
	Matrix44 result = matrix::multiply(orientation, translation);

	return result;	
}

vector3 get_forward(Camera const * camera)
{
	return camera->direction;
}

vector3 get_up(Camera const * camera)
{
	return vector::cross(get_right(camera), camera->direction);
}

vector3 get_right(Camera const * camera)
{
	using namespace vector;
	return normalize(cross(camera->direction, world::up));
}