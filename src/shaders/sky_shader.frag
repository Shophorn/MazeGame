#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

// layout(	binding = SAMPLER_BIND_ID, set = SAMPLER_SET_ID) uniform samplerCube texSampler;
layout(	binding = 0, set = 1) uniform sampler2D skyGradients[2];

layout (set = 3, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 ignored_color;
	vec4 ignored_ambient;
	vec4 ignored_cameraPosition;
	float skyColourSelection;
} light;

#include "skyfunc.glsl"

void main()
{
	vec3 lightDir 	= -normalize(light.direction.xyz);
	vec3 normal 	= normalize(fragTexCoord);

	outColor.rgb 	= compute_sky_color(normal, lightDir);

	outColor.a 		= 1.0;
}