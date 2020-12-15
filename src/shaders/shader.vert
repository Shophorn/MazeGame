#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
	float shadowDistance;
	float shadowTransitionDistance;
} camera;

layout(set = 2, binding = 0) uniform ModelProjection
{
	mat4 model;
} model;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inTangent;
layout (location = 3) in vec2 inTexCoord;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec3 fragPosition;
layout (location = 4) out vec4 lightCoords;
layout (location = 5) out mat3 tbnMatrix;
// layout (location = 6) out vec3 fragTangent;

const float shadowDistance = 90.0;
const float transitionDistance = 10.0;

void main ()
{
	gl_Position = camera.projection * camera.view * model.model * vec4(inPosition, 1.0);

	fragNormal 	= (transpose(inverse(model.model)) * vec4(inNormal, 0)).xyz;
	// fragTangent = (transpose(inverse(model.model)) * vec4(inTangent, 0)).xyz;

	// Todo(Leo): Check for mirrored faces with some hacks like -1 on tangent w component etc.
	// world space tangent, normal and bitangent
	vec3 t = normalize((model.model * vec4(inTangent, 0)).xyz);
	vec3 n = normalize((model.model * vec4(inNormal, 0)).xyz);
	vec3 b = normalize(cross(n, t));
	tbnMatrix = transpose(mat3(t, b, n));

	lightCoords = camera.lightViewProjection * model.model * vec4(inPosition, 1.0);
	lightCoords.xy *= 0.5;
	lightCoords.xy -= 0.5;

	vec4 worldPosition = model.model * vec4(inPosition, 1.0);

	fragPosition = worldPosition.xyz;
	
	float distance = length((camera.view * worldPosition).xyz);
	distance = distance - (shadowDistance - transitionDistance);
	distance = distance / transitionDistance;
	lightCoords.w = clamp (1.0 - distance, 0, 1);

	// fragColor 		= inColor;
	fragTexCoord 	= inTexCoord;
}

