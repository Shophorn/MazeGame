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
layout(location = 5) in mat3 tbnMatrix;

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
#if 1
	vec3 lightDir = tbnMatrix * -light.direction.xyz;
	vec3 normal = texture(texSampler[NORMAL_INDEX], fragTexCoord).xyz * 2.0 - 1.0;
	// normal.z *= 0.5;
	// normal.y = -normal.y;
	normal = normalize(normal);
#else
	vec3 lightDir = -light.direction.xyz;
	vec3 normal = fragNormal;
#endif
	// float ldotn = dot(-light.direction.xyz, normal);
	float ldotn = max(0, dot(lightDir, normal));

	float lightIntensity = ldotn;
	// float lightIntensity = smoothstep(0.2, 0.8, ldotn);
	// float lightIntensity = smoothstep(-0.05, 0.05, ldotn);
	// lightIntensity = mix(ldotn, lightIntensity, 0);
	// float lightIntensity = mix(ldotn, step(0, ldotn), 0.9);


	vec3 albedo = texture(texSampler[ALBEDO_INDEX], fragTexCoord).rgb;
	/// DEBUG ALBEDO
	// albedo = vec3(0.9,0.9,0.9);

	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z) * 0.8;

	// SHADOWS
	// lightIntensity = min(lightIntensity, inLight);	
	lightIntensity = lightIntensity * inLight;

	// lightIntensity *= 2;
	vec3 lightColor = mix(light.ambient.rgb, light.color.rgb, lightIntensity);
	// vec3 lightColor = light.ambient.rgb + light.color.rgb * lightIntensity;
	
	vec3 color = lightColor * albedo;

	outColor.rgb = color;
	outColor.a = 1.0;
}