#version 450

layout (set = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout(set = 2) uniform ModelProjection
{
	mat4 model;
} model;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 fragNormal;

void main ()
{
	// gl_Position = camera.projection * model * vec4(inPosition, 1.0);
	gl_Position = camera.projection * camera.view * model.model * vec4(inPosition, 1.0);

	// Todo(Leo): Check correctness???
	fragNormal = (transpose(inverse(model.model)) * vec4(inNormal, 0)).xyz;
	
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}

