#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout(location = 0) in vec3 inPosition;

// Note(Leo): this could also be passed as uniform, but it does not currently matter too much
layout(location = 1) in vec3 inColor;

layout (location = 0) out vec3 fragColor;

void main ()
{
	gl_Position 	= camera.projection * camera.view * vec4(inPosition, 1.0);
	fragColor 		= inColor;
}