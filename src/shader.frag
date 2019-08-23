#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

// Set these from c++ side
const int ALBEDO_INDEX = 0;
const int METALLIC_INDEX = 1;
const int SAMPLER_BIND_ID = 0;

layout(binding = SAMPLER_BIND_ID, set = 1) uniform sampler2D texSampler[2];

const vec3 lightDirection = normalize(vec3(0.7, -1, 0.3));
const float ambientIntensity = 0.5;

void main()
{
	float intensity = dot(-lightDirection, fragNormal) + ambientIntensity;

	outColor.rgb = fragNormal * 0.5 + 0.5;
	outColor.rgb = fragColor * intensity;




	int texIndex = fragTexCoord.x < 0.5 ? ALBEDO_INDEX : METALLIC_INDEX;
	outColor.rgb = texture (texSampler[texIndex], fragTexCoord).rgb * fragColor *intensity;

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;


	vec3 metalColor = vec3(0.67, 0.7, 0.75);
	outColor.rgb = mix(albedo, metalColor, metallic) * intensity;


	outColor.a = 1.0;
}