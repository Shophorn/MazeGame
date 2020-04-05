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

void main()
{
	float ldotn = dot(-light.direction.xyz, fragNormal);
	float lightIntensity = smoothstep(-0.05, 0.05, ldotn);
	lightIntensity = mix(ldotn, lightIntensity, 0.85);
	// float lightIntensity = mix(ldotn, step(0, ldotn), 0.9);

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;

	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);


	if (inLight < 0.5)
	{
		outColor = vec4(0,0,0.8,1);
		lightIntensity *= (1.0 - lightCoords.w);
		return;
	}

	vec3 light = light.ambient.rgb + (light.color.rgb * lightIntensity);
	vec3 color = light * albedo;

	// outColor.rgb = mix(color, normalize(camera.view[3].rgb), 0.3);
	outColor.rgb = color;
	outColor.a = 1.0;
}