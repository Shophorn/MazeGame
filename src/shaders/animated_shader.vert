#version 450

layout (set = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
} camera;

layout(set = 2) uniform ModelData
{
	mat4 model;
	mat4 boneTransforms [50];
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
	// mat4 boneTransforms [20] = 
	// {
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	// mat4 (3,0,0,0, 0,3,0,0, 0,0,3,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),

	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
		
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
		
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// 	mat4 (1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1),
	// };

	vec4 boneWeights = (inBoneWeights);

	mat4 skinMatrix = 
		boneWeights.x * model.boneTransforms[inBoneIndices.x] +
		boneWeights.y * model.boneTransforms[inBoneIndices.y] +
		boneWeights.z * model.boneTransforms[inBoneIndices.z] +
		boneWeights.w * model.boneTransforms[inBoneIndices.w];

	vec4 posePosition = skinMatrix * vec4(inPosition, 1);
	// posePosition = vec4(inPosition, 1);


	// const uint testIndex = 1;

	// if (inBoneIndices.x == testIndex) {	posePosition.z += inBoneWeights.x; }
	// if (inBoneIndices.y == testIndex) {	posePosition.z += inBoneWeights.y; }
	// if (inBoneIndices.z == testIndex) {	posePosition.z += inBoneWeights.z; }
	// if (inBoneIndices.w == testIndex) {	posePosition.z += inBoneWeights.w; }

	// fragColor = vec3(0,0,0);
	// if (inBoneIndices.x == testIndex) {	fragColor.r = inBoneWeights.x; }
	// if (inBoneIndices.y == testIndex) {	fragColor.g = inBoneWeights.y; }
	// if (inBoneIndices.z == testIndex) {	fragColor.b = inBoneWeights.z; }
	// // if (inBoneIndices.w == testIndex) {	fragColor.r = inBoneWeights.w; }


	gl_Position = camera.projection * camera.view * model.model * posePosition;

	// Todo(Leo): Check correctness???
	fragNormal = (transpose(inverse(model.model)) * vec4(inNormal, 0)).xyz;
	

	lightCoords = camera.lightViewProjection * model.model * vec4(inPosition, 1.0);
	lightCoords.xy *= 0.5;
	lightCoords.xy -= 0.5;

	vec4 worldPosition = model.model * vec4(inPosition, 1.0);
	float distance = length((camera.view * worldPosition).xyz);
	distance = distance - (shadowDistance - transitionDistance);
	distance = distance / transitionDistance;
	lightCoords.w = clamp (1.0 - distance, 0, 1);


	fragColor = inColor;
	fragTexCoord = inTexCoord;
}

