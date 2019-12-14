#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

// TODO(Leo): Set these from c++ side
const int ALBEDO_INDEX = 0;
const int METALLIC_INDEX = 1;
const int TEST_MASK_INDEX = 2;
const int TEXTURE_COUNT = 3;

const int SAMPLER_BIND_ID = 0;

layout(binding = SAMPLER_BIND_ID, set = 1) uniform sampler2D texSampler[TEXTURE_COUNT];

const vec3 lightDirection = normalize(vec3(0.7, 0.3, -2));
const float ambientIntensity = 0.2;

void main()
{
	float ldotn = dot(-lightDirection, fragNormal);


	float diffuse = ldotn;
	float intensity = diffuse + ambientIntensity;

	// float intensity = mix(ambientIntensity, 1.0f, step(0.5, (ldotn * 0.5 + 0.5)));

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;


	vec3 metalColor = albedo;//vec3(0.67, 0.7, 0.75);
	vec3 color = mix(albedo, metalColor, metallic) * intensity;

	float testMask = texture(texSampler[TEST_MASK_INDEX], fragTexCoord).r;
	vec3 testColor = fragColor * intensity;

	outColor.rgb = color;//mix (color, testColor, testMask);

	outColor.a = 1.0;
}