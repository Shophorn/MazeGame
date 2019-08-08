#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;


const vec3 lightDirection = normalize(vec3(0.7, -1, 0.3));
const float ambientIntensity = 0.15;

void main()
{
	float intensity = dot(-lightDirection, fragNormal) + ambientIntensity;

	outColor.rgb = texture (texSampler, fragTexCoord).rgb * fragColor *intensity;
	outColor.rgb = fragNormal * 0.5 + 0.5;
	outColor.rgb = fragColor * intensity;
	outColor.a = 1.0;
}