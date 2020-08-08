#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

// layout(	binding = SAMPLER_BIND_ID, set = SAMPLER_SET_ID) uniform samplerCube texSampler;
layout(	binding = 0, set = 1) uniform sampler2D texSampler[2];

layout (set = 3, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 ignored_color;
	vec4 ignored_ambient;
	vec4 ignored_cameraPosition;
	float skyColourSelection;
} lightInput;

#include "skyfunc.glsl"

void main()
{
	// vec3 light = -normalize(lightInput.direction.xyz);
	vec3 normal = normalize(fragTexCoord);
	// float d = dot(light, normal);

	// d = (d + 1) / 2;
	// vec3 color = mix(	texture(texSampler[0], vec2(max(0, d), 0)).rgb,
	// 					texture(texSampler[1], vec2(max(0, d), 0)).rgb,
	// 					lightInput.skyColourSelection);

	// outColor.rgb 	= color;
	outColor.rgb 	= compute_sky_color(normal);
	outColor.a 		= 1.0;
}