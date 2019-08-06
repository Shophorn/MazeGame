constexpr Vector3 vector3_world_up = {0.0f, 1.0f, 0.0f};
constexpr Vector3 vector3_zero = {0, 0, 0};

// Note(Leo): these need to align properly
struct CameraUniformBufferObject
{
	alignas(16) Matrix44 view;
	alignas(16) Matrix44 perspective;
};

struct Camera
{
	Vector3 target;
	Vector3 position;
	real32 distance;
	real32 rotation;

	real32 fieldOfView;
	real32 nearClipPlane;
	real32 farClipPlane;
	real32 aspectRatio;
};

internal Matrix44
Perspective(real32 fieldOfView, real32 aspectRatio, real32 nearClipPlane, real32 farClipPlane)
{
	// Todo(Leo): Do studies, this really affects many things
	// Study: https://www.scratchapixel.com/lessons/3d-basic-rendering/perspective-and-orthographic-projection-matrix/building-basic-perspective-projection-matrix

	constexpr real32 pi = 3.141592653589793f;

	Matrix44 result = {};

    // set the basic projection matrix
    float scale = 1.0f / Tan(fieldOfView * 0.5f * pi / 180.0f); 
    result[0][0] = scale; // scale the x coordinates of the projected point 
    result[1][1] = -scale; // scale the y coordinates of the projected point 
    result[2][2] = -farClipPlane / (farClipPlane - nearClipPlane); // used to remap z to [0,1] 
    result[3][2] = -farClipPlane * nearClipPlane / (farClipPlane - nearClipPlane); // used to remap z [0,1] 
    result[2][3] = -1; // set w = -z 
    result[3][3] = 0; 

	return result;
}

internal Matrix44
ViewProjection(Vector3 cameraPosition, Vector3 cameraTarget)
{
	// Study: https://www.3dgep.com/understanding-the-view-matrix/

	Vector3 forward = Normalize(cameraPosition - cameraTarget);
	Vector3 right 	= Normalize(Cross(vector3_world_up, forward));
	Vector3 up  	= Cross(forward, right);

	// Todo(Leo): translation can be done manually here too
	Matrix44 orientation = {
		right[0], up[0], forward[0], 0,
		right[1], up[1], forward[1], 0,
		right[2], up[2], forward[2], 0,
		0, 0, 0, 1
	};
	Matrix44 translation = Matrix44::Translate(-cameraPosition);

	Matrix44 result = orientation * translation;
	return result;	

}

CameraUniformBufferObject GetCameraUniforms(Camera * camera)
{
	CameraUniformBufferObject result;

	result.perspective = Perspective(	camera->fieldOfView, camera->aspectRatio,
										camera->nearClipPlane, camera->farClipPlane);

	result.view = ViewProjection(camera->position, camera->target);

	return result;
}

