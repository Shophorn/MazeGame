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

const vec3 lightDirection = normalize(vec3(0.7, 0.3, -1));
const float ambientIntensity = 0.4;

void main()
{
	float ldotn = dot(-lightDirection, fragNormal);

	float intensity = mix(ambientIntensity, 1.0f, step(0.5, (ldotn * 0.5 + 0.5)));

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;


	vec3 metalColor = vec3(0.67, 0.7, 0.75);
	outColor.rgb = mix(albedo, metalColor, metallic) * intensity;


	// outColor.rg = fragTexCoord;
	// outColor.b = 0;

	outColor.a = 1.0;
}