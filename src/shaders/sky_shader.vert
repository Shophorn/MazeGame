#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout (location = 0) in vec3 inPosition;
// layout (location = 1) in vec3 inNormal;
// layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 1) out vec3 fragTexCoord;

void main ()
{
	mat4 rotatedView = camera.view;
	rotatedView[3] = vec4(0,0,0,1);

	gl_Position = camera.projection * rotatedView * vec4(inPosition, 1.0);
	
	fragTexCoord = inPosition;
}

