constexpr Vector3 vector3_world_up = {0.0f, 1.0f, 0.0f};
constexpr Vector3 vector3_forward = {0.0f, 0.0f, 1.0f};
constexpr Vector3 vector3_zero = {0, 0, 0};

struct Camera
{
	Vector3 position;
	// Todo(Leo): change to quaternion
	Vector3 forward = vector3_forward;

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
	// Todo(Leo): Do studies, this really affects many things
	// Study: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix
    
	Matrix44 result = {};
    float scale = 1.0f / Tan(fieldOfView * 0.5f * pi / 180.0f); 
    result[0][0] = scale;
    result[1][1] = -scale * aspectRatio;
    result[2][2] = -farClipPlane / (farClipPlane - nearClipPlane);
    result[3][2] = -farClipPlane * nearClipPlane / (farClipPlane - nearClipPlane);
    result[2][3] = -1;
    result[3][3] = 0; 

	return result;	
}

Matrix44
Camera::ViewProjection()
{
	// Study: https://www.3dgep.com/understanding-the-view-matrix/

	Vector3 zAxis 	= -forward;
	Vector3 xAxis 	= Normalize(Cross(vector3_world_up, zAxis));
	Vector3 yAxis 	= Cross (zAxis, xAxis);

	// Todo(Leo): translation can be done manually here too
	Matrix44 orientation = {
		xAxis[0], yAxis[0], zAxis[0], 0,
		xAxis[1], yAxis[1], zAxis[1], 0,
		xAxis[2], yAxis[2], zAxis[2], 0,
		0, 0, 0, 1
	};
	Matrix44 translation = Matrix44::Translate(-position);

	Matrix44 result = orientation * translation;
	return result;	
}