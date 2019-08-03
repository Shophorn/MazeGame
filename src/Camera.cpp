constexpr glm::vec3 vector3_world_up = {0.0f, 1.0f, 0.0f};
constexpr glm::vec3 vector3_zero = {0, 0, 0};

struct Camera
{
	glm::vec3 target;
	float distance;
	float rotation;

	float fieldOfView;
	float nearClipPlane;
	float farClipPlane;
	float aspectRatio;

	glm::mat4 GetViewProjection()
	{
		// Todo(Leo): Obviously not correct >:(
		glm::mat4 result = glm::lookAt(glm::vec3(distance, distance, distance), target, vector3_world_up);
		return result;
	}

	glm::mat4 GetPerspectiveProjection()
	{
	   	/* Note(Leo): glm generates inverted y for opengl, so that needs to inverted.
        Also affects triangle winding so be careful */
		glm::mat4 result = glm::perspective(glm::radians(fieldOfView), aspectRatio, nearClipPlane, farClipPlane);
        result[1][1] *= -1;
		return result;
	}
};