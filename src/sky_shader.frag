#version 450

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int ALBEDO_INDEX = 0;
const int METALLIC_INDEX = 1;
const int TEST_MASK_INDEX = 2;
const int TEXTURE_COUNT = 3;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

layout(	binding = SAMPLER_BIND_ID,	
		set = SAMPLER_SET_ID
) uniform sampler2D texSampler[TEXTURE_COUNT];

void main()
{
	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;

	outColor.rgb = albedo;
	outColor.b = 1.0;
	outColor.a = 1.0;
}