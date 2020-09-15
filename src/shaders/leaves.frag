#version 450
#extension GL_GOOGLE_include_directive : enable

const int LIGHTING_SET 		= 1;
const int SKY_GRADIENT_SET 	= 4;
#include "skyfunc.glsl"

layout(binding = 0, set = 2) uniform sampler2D lightMap;
layout(binding = 0, set = 3) uniform sampler2D maskTexture;

layout(push_constant) uniform ColorBlock
{
	vec3 colour;
} colour;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 lightCoords;
layout(location = 3) in vec3 fragPosition;

layout(location = 0) out vec4 outColor;

// Todo(Leo): use gradient textures
const vec3 darkerColours [] =
{
	vec3(0.2, 0.05, 0.4),
	vec3(0.2, 0.32, 0.5),
	vec3(0.32, 0.50, 0.25),
	vec3(0.8, 0.5, 0.6),
};

const vec3 lighterColours [] = 
{
	vec3(0.62, 0.3, 0.8),
	vec3(0.3, 0.62, 0.8),
	vec3(0.47, 0.7, 0.40),
	vec3(1.0, 0.7, 0.8),
};

const vec3 putple = vec3(0.62, 0.3, 0.8);
const vec3 bluish = vec3(0.3, 0.62, 0.8);
const vec3 green = vec3(0.52, 0.7, 0.40);
const vec3 pinkt = vec3(1.0, 0.7, 0.8);

const vec3 darkerGreen = vec3(0.32, 0.50, 0.25);
const vec3 lighterGreen = vec3(0.47, 0.7, 0.4);

void main()
{
	const float gamma = 2.2;

	#if 0

	float distanceFromCenter 	= length(fragTexCoord.xy * 2);
	float opacity 				= step (distanceFromCenter, 1.0);

	const float angle = 0.2;

	float x = fragTexCoord.x * 2;
	float y = fragTexCoord.y * 2;

	float op2 = angle * y + angle;
	opacity *= step(abs(op2), abs(x));
	#else

	float opacity = texture(maskTexture, fragTexCoord).r;
 	#endif

	if (opacity < 0.5)
	{
		discard;
	}

	vec3 lightDir 	= -light.direction.xyz;
	vec3 normal 	= normalize(fragNormal);

	// Todo(Leo): Add translucency
	// float ldotn = dot(lightDir, normal);
	float ldotn = (dot(lightDir, normal));

	ldotn = abs(ldotn);
	// ldotn = max(0, ldotn);
	vec3 albedo = colour.colour;

	albedo = pow(albedo, vec3(gamma));

	float lightIntensity = ldotn;


	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float shadowBias = 0.001;
	float inLight = 1.0 - step(lightDepthFromTexture + shadowBias, lightCoords.z);// * 0.8;

	// SHADOWS
	lightIntensity = lightIntensity * inLight;	

	// Note(Leo): try these two
	// vec3 lightColor = mix(light.ambient.rgb, light.color.rgb, lightIntensity);
	// vec3 lightColor 	= compute_sky_color(normal, lightDir);
	// vec3 diffuseTerm 	= mix(light.ambient.rgb, lightColor, lightIntensity) * albedo;

	vec3 viewDirection 		= normalize(light.cameraPosition.xyz - fragPosition);

	float smoothness 		= 0.5; //material.smoothness;
	float specularStrength 	= 0.1;//material.specularStrength;;
	vec3 halfVector 		= normalize(lightDir + viewDirection);
	float spec 				= max(0, dot (halfVector, normal));
	spec 					= pow(spec, 256 * smoothness);

	const float epsilon = 0.0001;

	float inLightActual 	= 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);
	// vec3 specularTerm 		= spec * specularStrength * lightColor * inLightActual;


	// vec3 lightColor = light.ambient.rgb + light.color.rgb * lightIntensity;

	SunAndAmbientLights lights = BETTER_compute_sky_color(normal);
	vec3 diffuseTerm = albedo * lights.sun * lightIntensity;	
	vec3 ambientTerm = albedo * lights.ambient;

	
	// vec3 color = diffuseTerm + specularTerm;


	// color *= step(distanceFromCenter, 1.0);

	outColor.rgb = diffuseTerm + ambientTerm;
	outColor.a 					= 1;
}