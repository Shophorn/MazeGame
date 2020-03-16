#version 450

layout (set = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout (set = 3) uniform Lighting
{
	vec4 direction;
	vec4 color;
	vec4 ambient;
} light;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec4 lightCoords;

layout(location = 0) out vec4 outColor;

// TODO(Leo): Set these from c++ side
const int ALBEDO_INDEX = 0;
const int METALLIC_INDEX = 1; 
const int TEST_MASK_INDEX = 2;
const int TEXTURE_COUNT = 3;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

layout(	binding = SAMPLER_BIND_ID,	
		set = SAMPLER_SET_ID
) uniform sampler2D texSampler[TEXTURE_COUNT];

layout(binding = 0, set = 4) uniform sampler2D lightMap;

float linearize_depth(float depth, float near, float far)
{
	float n = near; // camera z near
	float f = far; // camera z far
	float z = depth;
	return (2.0 * n) / (f + n - z * (f - n));	
}

const vec3 foreground = vec3 (0.45, 0.4, 0.45); //vec3(0.9, 0.85, 0.85);
const vec3 background = vec3 (0.8, 0.7, 0.9); //vec3(0.2, 0.1, 0.3);

void main()
{
	float ldotn = dot(-light.direction.xyz, fragNormal);
	float lightIntensity = smoothstep(-0.05, 0.05, ldotn);
	lightIntensity = mix(ldotn, lightIntensity, 0.85);
	// float lightIntensity = mix(ldotn, step(0, ldotn), 0.9);

	float test = linearize_depth(gl_FragCoord.z, 0.1, 1000);
	test = clamp(test * 3, 0, 1);
	vec3 albedo = mix(foreground, background, test);
	albedo *= texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;


	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);

	if (inLight < 0.5)
	{
		lightIntensity *= (1.0 - lightCoords.w);
	}


	// /float factor = inLight * lightCoords.w;











	// lightIntensity *= inLight;

	// lightIntensity = factor;

	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;

	vec3 light = light.ambient.rgb + (light.color.rgb * lightIntensity);

	outColor.rgb = light * albedo;


	// outColor.rg = mix(outColor.rg, lightCoords.xy, 0.5);
	// outColor.b = lightCoords.z;


	outColor.a = 1.0;
}