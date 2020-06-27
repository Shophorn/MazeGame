#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
	float shadowDistance;
	float shadowTransitionDistance;
} camera;

layout (location = 0) in mat4 modelMatrix;

layout (location = 0) out vec2 fragTexCoord;
layout (location = 1) out vec3 fragNormal;
layout (location = 2) out vec4 lightCoords;

const float shadowDistance = 90.0;
const float transitionDistance = 10.0;

const uint vertexCount = 4;

const vec2 vertexTexcoords [vertexCount] =
{
	vec2 (-0.5, -0.5),
	vec2 ( 0.5, -0.5),
	vec2 (-0.5,  0.5),
	vec2 ( 0.5,  0.5),
};

const vec3 vertexPositions [vertexCount] =
{
	vec3 (-0.5, 0, 0),
	vec3 (0.5, 0, 0),
	vec3 (-0.5, 1, 0),
	vec3 (0.5, 1, 0)
};

const vec3 vertexNormals[vertexCount] =
{
	vec3(0,0,1),
	vec3(0,0,1),
	vec3(0,0,1),
	vec3(0,0,1),
	// normalize(vec3(-0.2, -0.3, 0.5)),
	// normalize(vec3( 0.2, -0.3, 0.5)),
	// normalize(vec3(-0.2, 0.3, 0.5)),
	// normalize(vec3( 0.2, 0.3, 0.5)),
};	

void main ()
{
	vec3 inPosition = vertexPositions[gl_VertexIndex];
	vec3 inNormal 	= vertexNormals[gl_VertexIndex];


	// gl_Position = camera.projection * camera.view * model.model * vec4(inPosition, 1.0);
	gl_Position = camera.projection * camera.view * modelMatrix * vec4(inPosition, 1.0);

	// Todo(Leo): Check correctness???
	// fragNormal 	= (transpose(inverse(model.model)) * vec4(inNormal, 0)).xyz;
	fragNormal 	= (transpose(inverse(modelMatrix)) * vec4(inNormal, 0)).xyz;

	// // world space tangent, normal and bitangent
	// vec3 t = normalize((model.model * vec4(inTangent, 0)).xyz);
	// vec3 n = normalize((model.model * vec4(inNormal, 0)).xyz);
	// vec3 b = normalize(cross(n, t));
	// tbnMatrix = transpose(mat3(t, b, n));

	lightCoords = camera.lightViewProjection * modelMatrix * vec4(inPosition, 1.0);
	lightCoords.xy *= 0.5;
	lightCoords.xy -= 0.5;

	vec4 worldPosition = modelMatrix * vec4(inPosition, 1.0);
	float distance = length((camera.view * worldPosition).xyz);
	distance = distance - (shadowDistance - transitionDistance);
	distance = distance / transitionDistance;
	lightCoords.w = clamp (1.0 - distance, 0, 1);

	fragTexCoord 	= vertexTexcoords[gl_VertexIndex].xy;
}