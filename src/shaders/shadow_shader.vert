#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view_;
	mat4 projection_;
	mat4 lightViewProjection;
} camera;

layout(set = 1, binding = 0) uniform ModelData
{
	mat4 localToWorld;
	float isAnimated;
	mat4 bonesToLocal[32];
} model;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec2 inTexCoord;

layout (location = 4) in uvec4 inBoneIndices;
layout (location = 5) in vec4 inBoneWeights;

// layout (location = 0) out vec3 fragColor;
// layout (location = 1) out vec2 fragTexCoord;
// layout (location = 2) out vec3 fragNormal;

void main ()
{
	if (model.isAnimated < 0.5)
	{
		gl_Position = camera.lightViewProjection * model.localToWorld * vec4(inPosition, 1.0);
	}
	else
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

		gl_Position = camera.lightViewProjection * model.localToWorld * posePosition;
	}




	// // Todo(Leo): Check correctness???
	// fragNormal = (transpose(inverse(model.model)) * vec4(inNormal, 0)).xyz;
	
	// fragColor = inColor;
	// fragTexCoord = inTexCoord;
}

