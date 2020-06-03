#version 450

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
// layout(location = 5) in mat3 tbnMatrix;

layout(location = 0) out vec4 outColor;

// TODO(Leo): Set these from c++ side
// const int ALBEDO_INDEX = 0;
// const int NORMAL_INDEX = 1; 
// const int TEST_MASK_INDEX = 2;
// const int TEXTURE_COUNT = 3;

// const int SAMPLER_BIND_ID = 0;
// const int SAMPLER_SET_ID = 1;

// layout(	binding = SAMPLER_BIND_ID,	
// 		set = SAMPLER_SET_ID
// ) uniform sampler2D texSampler[TEXTURE_COUNT];

layout(binding = 0, set = 4) uniform sampler2D lightMap;

void main()
{
	vec3 lightDir = -light.direction.xyz;
	vec3 normal = fragNormal;

	// float ldotn = dot(-light.direction.xyz, normal);
	float ldotn = max(0, dot(lightDir, normal));

	float lightIntensity = ldotn;
	// float lightIntensity = smoothstep(0.2, 0.8, ldotn);
	// float lightIntensity = smoothstep(-0.05, 0.05, ldotn);
	// lightIntensity = mix(ldotn, lightIntensity, 0);
	// float lightIntensity = mix(ldotn, step(0, ldotn), 0.9);

	vec3 albedo = fragColor;

	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);

	// SHADOWS
	lightIntensity = min(lightIntensity, inLight);	

	// lightIntensity *= 2;
	vec3 lightColor = mix(light.ambient.rgb, light.color.rgb, lightIntensity);
	// vec3 lightColor = light.ambient.rgb + light.color.rgb * lightIntensity;
	
	vec3 color = lightColor * albedo;

	float distanceFromCenter 	= length(fragTexCoord.xy * 2);
	color *= step(distanceFromCenter, 1.0);
	outColor.rgb = color;

	float opacity 				= step (distanceFromCenter, 1.0);

	if (opacity < 0.5)
	{
		discard;
	}

	outColor.a 					= 1;
}

// #version 450

// layout(location = 0) in vec4 fragColor;
// layout(location = 1) in vec2 fragTexCoord;

// layout(location = 0) out vec4 outColor;

// const int SAMPLER_BIND_ID = 0;

// layout(binding = SAMPLER_BIND_ID, set = 0) uniform sampler2D texSampler;

// void main()
// {
// 	vec4 tex 		= texture(texSampler, fragTexCoord);
// 	outColor.rgb 	= fragColor.rgb * tex.rgb;
// 	outColor.a 		= fragColor.a * tex.a;
// }