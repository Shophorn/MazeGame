#version 450

layout (set = 0, binding = 0) uniform CameraProjetions
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
} camera;

const vec3 positions [4] =
{
	vec3(-0.5, 0, 0);	
	vec3(0.5, 0, 0);	
	vec3(-0.5, 1, 0);	
	vec3(0.5, 1, 0);	
};

void main()
{
	gl_Position = camera.projection * camera.view * 
}