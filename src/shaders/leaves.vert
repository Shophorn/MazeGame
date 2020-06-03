#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
	float shadowDistance;
	float shadowTransitionDistance;
} camera;

// layout(set = 2, binding = 0) uniform ModelProjection
// {
// 	mat4 model;
// } model;

// layout (location = 0) in vec3 inPosition;
// layout (location = 1) in vec3 inNormal;
// layout (location = 2) in vec3 inColor;
// layout (location = 3) in vec2 inTexCoord;
// layout (location = 6) in vec3 inTangent;
// layout (location = 7) in vec3 inBiTangent;

layout (location = 0) in mat4 modelMatrix;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec3 fragPosition;
layout (location = 4) out vec4 lightCoords;
// layout (location = 5) out mat3 tbnMatrix;

const float shadowDistance = 90.0;
const float transitionDistance = 10.0;

const vec3 vertexPositions [4] =
{
	vec3(-0.5, -0.5, 0),
	vec3( 0.5, -0.5, 0),
	vec3(-0.5,  0.5, 0),
	vec3( 0.5,  0.5, 0),
};

void main ()
{
	vec3 inPosition = vertexPositions[gl_VertexIndex];
	vec3 inNormal = vec3(0, 0, 1);

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


	// Putple
	// fragColor 		= vec3(0.62, 0.3, 0.8);

	// Bluish
	// fragColor 		= vec3(0.3, 0.62, 0.8);

	// Green
	fragColor 		= vec3(0.52, 0.7, 0.40);
	fragTexCoord 	= vertexPositions[gl_VertexIndex].xy;
}