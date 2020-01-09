#version 450

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

layout(	binding = SAMPLER_BIND_ID, set = SAMPLER_SET_ID) uniform samplerCube texSampler;

void main()
{
	vec4 albedo = texture(texSampler, fragTexCoord);

	outColor = albedo;
	// outColor.a = 1.0;
}