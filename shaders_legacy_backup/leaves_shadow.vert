#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view_;
	mat4 projection_;
	mat4 lightViewProjection;
} camera;

layout(location = 0) in mat4 modelMatrix;
layout (location = 1) out vec2 fragTexCoord;

const uint vertexCount = 4;

const vec3 vertexPositions [vertexCount] =
{
	vec3(-0.5, -0.5, 0),
	vec3( 0.5, -0.5, 0),
	vec3(-0.5,  0.5, 0),
	vec3( 0.5,  0.5, 0),
};

const vec2 vertexTexcoords [vertexCount] =
{
	vec2 (0, 0),
	vec2 (1, 0),
	vec2 (0, 1),
	vec2 (1, 1),
};

void main ()
{
	vec3 inPosition = vertexPositions[gl_VertexIndex];
	gl_Position = camera.lightViewProjection * modelMatrix * vec4(inPosition, 1.0);

	fragTexCoord 	= vertexTexcoords[gl_VertexIndex].xy;
}