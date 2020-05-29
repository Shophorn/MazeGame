#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;

layout(binding = SAMPLER_BIND_ID, set = 0) uniform sampler2D texSampler;

void main()
{
	vec4 tex 		= texture(texSampler, fragTexCoord);
	outColor.rgb 	= fragColor.rgb * tex.rgb;
	outColor.a 		= fragColor.a * tex.a;
}