#version 450

layout(push_constant) uniform Color
{
	layout(offset=64) vec4 color;
} color;

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 0) uniform sampler2D texSampler;


void main()
{
	vec4 tex 		= texture(texSampler, fragTexCoord);
	outColor.rgb 	= color.color.rgb * tex.rgb;
	outColor.a 		= color.color.a * tex.a;

	// float gamma = 2.2;
	// outColor.rgb = pow(outColor.rgb, vec3(1/gamma));
}