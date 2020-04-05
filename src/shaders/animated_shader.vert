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
	mat4 skinMatrix = 
		inBoneWeights.x * model.bonesToLocal[inBoneIndices.x] +
		inBoneWeights.y * model.bonesToLocal[inBoneIndices.y] +
		inBoneWeights.z * model.bonesToLocal[inBoneIndices.z] +
		inBoneWeights.w * model.bonesToLocal[inBoneIndices.w];

	// Note(Leo): position coordinates must be divided with z-component after matrix multiplication
	vec4 posePosition = skinMatrix * vec4(inPosition, 1);
	posePosition /= posePosition.w;

	vec4 poseNormal = skinMatrix * vec4(inNormal, 0);

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
	float red = posePosition.w;
	// red = (red - 1) * 5;
	fragColor = vec3(1, 1 - red, 1 - red);

	fragColor = mod((model.localToWorld * posePosition).xyz / 10, 1.0);
	fragTexCoord = inTexCoord;
}

