#version 450

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

// const uint maxGradient = 5;

// const uint niceSkyGradientCount = 4;
// const vec4 niceSkyGradient [maxGradient] =
// {
// 	vec4(0.43 / 2.55, 0.3/2.55, 0.98/2.55,  0.1),
// 	vec4(0.4, 0.9, 1.0,						0.3),
// 	vec4(0.8, 0.99, 1.0,					0.99),
// 	vec4(1,1,1, 							0.999),
// 	vec4(0),
// };

// const uint meanSkyGradientCount = 5;
// const vec4 meanSkyGradient [maxGradient] =
// {
// 	vec4(0,0,0,  			0.1),
// 	vec4(0.8, 0.1, 0.05,	0.3),
// 	vec4(0.8, 0.1, 0.05,	0.7),
// 	vec4(1.0, 0.95, 0.8,	0.99),
// 	vec4(1,1,1, 			0.999),
// };

// vec3 evaluateSkyLight (uint count, vec4 gradient[maxGradient], float time)
// {
// 	if(time < gradient[0].w)
// 		return gradient[0].rgb;

// 	if(time > gradient[count -1].w)
// 		return gradient[count - 1].rgb;

// 	for(int i = 1; i < count; ++i)
// 	{
// 		if (time < gradient[i].w)
// 		{
// 			float t = (time - gradient[i - 1].w) / (gradient[i].w - gradient[i - 1].w);
// 			return mix(gradient[i - 1].rgb, gradient[i].rgb, t);
// 		}
// 	}
// }

void main()
{
	vec3 light = -normalize(lightInput.direction.xyz);
	vec3 normal = normalize(fragTexCoord);
	float d = dot(light, normal);

	d = (d + 1) / 2;
	vec3 color = mix(	texture(texSampler[0], vec2(max(0, d), 0)).rgb,
						texture(texSampler[1], vec2(max(0, d), 0)).rgb,
						// evaluateSkyLight(meanSkyGradientCount, meanSkyGradient, d),
						lightInput.skyColourSelection);

	outColor.rgb 	= color;
	outColor.a 		= 1.0;
}