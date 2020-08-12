#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout(push_constant) uniform LineInfo
{
	vec4 color;
} line;

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 fragColor;

void main ()
{
	gl_Position = camera.projection * camera.view * vec4(position, 1.0);
	fragColor 	= line.color.rgb;
}

