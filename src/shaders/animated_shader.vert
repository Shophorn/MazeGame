#version 450

layout (set = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
} camera;

layout(set = 2) uniform ModelData
{
	mat4 localToWorld;
	float isAnimated;
	mat4 bonesToLocal [32];
} model;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;
layout (location = 4) in uvec4 inBoneIndices;
layout (location = 5) in vec4 inBoneWeights;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;
layout (location = 2) out vec3 fragNormal;
layout (location = 3) out vec4 lightCoords;

// Todo(Leo): Move to uniform lighting section
const float shadowDistance = 90.0;
const float transitionDistance = 10.0;

void main ()
{
	// This does not work
	mat4 skinMatrix =
		inBoneWeights[0] * model.bonesToLocal[inBoneIndices[0]] +
		inBoneWeights[1] * model.bonesToLocal[inBoneIndices[1]] +
		inBoneWeights[2] * model.bonesToLocal[inBoneIndices[2]] +
		inBoneWeights[3] * model.bonesToLocal[inBoneIndices[3]];

	skinMatrix /= skinMatrix[3][3];

	// Note(Leo): position coordinates must be divided with z-component after matrix multiplication
	vec4 posePosition = skinMatrix * vec4(inPosition, 1);
	// posePosition /= posePosition.w;

	vec4 poseNormal = skinMatrix * vec4(inNormal, 0);
	poseNormal = normalize(poseNormal); // Is this necessary?

	gl_Position = camera.projection * camera.view * model.localToWorld * posePosition;
	// vec4 poseNormal = vec4(inNormal, 0);
	fragNormal 		= (transpose(inverse(model.localToWorld)) * poseNormal).xyz;
	
	lightCoords = camera.lightViewProjection * model.localToWorld * posePosition;//vec4(inPosition, 1.0);
	lightCoords.xy *= 0.5;
	lightCoords.xy -= 0.5;

	vec4 worldPosition = model.localToWorld * posePosition;//vec4(inPosition, 1.0);
	float distance = length((camera.view * worldPosition).xyz);
	distance = distance - (shadowDistance - transitionDistance);
	distance = distance / transitionDistance;
	lightCoords.w = clamp (1.0 - distance, 0, 1);

	fragColor = inColor;
	fragTexCoord = inTexCoord;
}

