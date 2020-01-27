
#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;

layout(binding = SAMPLER_BIND_ID, set = 0) uniform sampler2D texSampler;

float LinearizeDepth(float depth)
{
	float n = 0.1; // camera z near
	float f = 1000; // camera z far
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

void main()
{
	float depth = texture(texSampler, fragTexCoord).r;
	// depth = LinearizeDepth(depth);
	outColor.rgb = depth.rrr; //fragColor * texture(texSampler, fragTexCoord).rgb;
	// outColor.rgb = 0.5.rrr;
	outColor.a = 1.0;
}


// void main() 
// {
// 	float depth = texture(samplerColor, inUV).r;
// 	outFragColor = vec4(vec3(1.0-LinearizeDepth(depth)), 1.0);
// }