#version 450
#extension GL_GOOGLE_include_directive : enable

const int LIGHTING_SET 		= 3;
const int SKY_GRADIENT_SET 	= 1;
#include "skyfunc.glsl"

layout(location = 1) in vec3 fragTexCoord;
layout(location = 0) out vec4 outColor;

void main()
{
	vec3 lightDir 	= -normalize(light.direction.xyz);
	vec3 normal 	= normalize(fragTexCoord);

	outColor.rgb 	= compute_sky_color(normal);

	outColor.a 		= 1.0;
}