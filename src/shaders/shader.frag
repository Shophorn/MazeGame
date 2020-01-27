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
layout(location = 3) in vec4 shadowCoords;


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

void main()
{
	float ldotn = dot(-light.direction.xyz, fragNormal);
	float ligthIntensity = smoothstep(-0.05, 0.05, ldotn);
	ligthIntensity = mix(ldotn, ligthIntensity, 0.85);
	// float ligthIntensity = mix(ldotn, step(0, ldotn), 0.9);

	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	float metallic = texture(texSampler[METALLIC_INDEX], fragTexCoord).r;

	vec3 light = light.ambient.rgb + (light.color.rgb * ligthIntensity);
	vec3 color = light * albedo;

	// outColor.rgb = mix(color, normalize(camera.view[3].rgb), 0.3);
	outColor.rgb = color;
	outColor.a = 1.0;
}