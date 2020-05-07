#version 450

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout (set = 3, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 color;
	vec4 ambient;
} light;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in vec4 lightCoords;


layout(location = 0) out vec4 outColor;

// TODO(Leo): Set these from c++ side
const int ALBEDO_INDEX = 0;
const int NORMAL_INDEX = 1; 
const int TEST_MASK_INDEX = 2;
const int TEXTURE_COUNT = 3;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

layout(	binding = SAMPLER_BIND_ID,	
		set = SAMPLER_SET_ID
) uniform sampler2D texSampler[TEXTURE_COUNT];

layout(binding = 0, set = 4) uniform sampler2D lightMap;

void main()
{
	vec3 normal = fragNormal;

	float ldotn = dot(-light.direction.xyz, normal);
	float lightIntensity = smoothstep(-0.05, 0.05, ldotn);
	lightIntensity = mix(ldotn, lightIntensity, 0.85);
	// float lightIntensity = mix(ldotn, step(0, ldotn), 0.9);

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;

	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);


	// outColor.xyz = fragColor;
	// outColor.a = 1;
	// return;

	// Luma, luminance:
	// Y = 0.299 R + 0.587 G + 0.114 B
	// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color

	// This is maybe luminance
	float luma = dot(albedo, vec3(0.299, 0.587, 0.114));

	// outColor.xyz = luma.xxx;
	// outColor.a = 1;
	// return;

	// const vec3 shadowColor = vec3 (0,0.1, 0.5);
	lightIntensity = min(lightIntensity, inLight);

	// vec3 light = light.ambient.rgb + (light.color.rgb * lightIntensity);
	vec3 color = light.color.rgb * albedo;
	if (lightIntensity < 0.5)
	{
		// color = light.ambient.rgb;//max(light.ambient.rgb, color);
		// color = max(light.ambient.rgb, color);
		color = light.ambient.rgb * albedo;
	}

	// outColor.rgb = mix(color, normalize(camera.view[3].rgb), 0.3);
	outColor.rgb = color;
	outColor.a = 1.0;


	// if (inLight < 0.5)
	// {
	// 	const vec3 shadowColor = vec3 (0,0.1, 0.5);
	// 	outColor.rgb *= shadowColor;

	// 	// outColor.rgb = mix(outColor.rgb, vec3(0,0.1,0.5), 0.9);


	// 	// outColor.xyz *= luma;
	// 	// lightIntensity *= (1.0 - lightCoords.w);
	// 	return;
	// }
}