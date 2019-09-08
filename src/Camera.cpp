struct Camera
{
	Vector3 position;
	// Todo(Leo): change to quaternion, maybe
	Vector3 forward = World::Forward;

	real32 fieldOfView;
	real32 nearClipPlane;
	real32 farClipPlane;
	real32 aspectRatio;

	void LookAt(Vector3 target);

	Matrix44 ViewProjection();
	Matrix44 PerspectiveProjection();
};

void
Camera::LookAt(Vector3 target)
{
	forward = Normalize(target - position);
}

Matrix44
Camera::PerspectiveProjection()
{
	/*
	Todo(Leo): Do studies, this really affects many things
	Study: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix

	Yay, works for now
	*/
	real32 canvasSize = Tan(ToRadians(fieldOfView / 2.0f)) * nearClipPlane;

	// Note(Leo): Vulkan NDC goes left-top = (-1, -1) to right-bottom = (1, 1)
	real32 bottom = canvasSize / 2;
	real32 top = -bottom;

	real32 right = bottom * aspectRatio;
	real32 left = -right;

    real32
    	b = bottom,
    	t = top,
    	r = right,
    	l = left,

    	n = nearClipPlane,
    	f = farClipPlane;


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
Camera::ViewProjection()
{
	// Study: https://www.3dgep.com/understanding-the-view-matrix/

	Vector3 zAxis 	= -forward;
	Vector3 xAxis 	= Normalize(Cross(World::Up, zAxis));
	Vector3 yAxis 	= Cross (zAxis, xAxis);

	Matrix44 orientation = {
		xAxis[0], yAxis[0], zAxis[0], 0,
		xAxis[1], yAxis[1], zAxis[1], 0,
		xAxis[2], yAxis[2], zAxis[2], 0,
		0, 0, 0, 1
	};
	
	/* Todo(Leo): Translation can be done inline with matrix initialization
	instead of as separate function call */
	Matrix44 translation = Matrix44::Translate(-position);
	Matrix44 result = orientation * translation;

	return result;	
}