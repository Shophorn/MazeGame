#version 450
#extension GL_GOOGLE_include_directive : enable


layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

const int LIGHTING_SET 		= 3;
const int SKY_GRADIENT_SET 	= 5;
#include "skyfunc.glsl"

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in vec4 lightCoords;
layout(location = 5) in mat3 tbnMatrix;

layout(location = 0) out vec4 outColor;

const int ALBEDO_INDEX 		= 0;
const int NORMAL_INDEX 		= 1; 
const int TEST_MASK_INDEX 	= 2;
const int TEXTURE_COUNT 	= 3;

layout(set = 1, binding = 0) uniform sampler2D texSampler[TEXTURE_COUNT];
layout(set = 4, binding = 0) uniform sampler2D lightMap;


layout (push_constant) uniform MaterialBlock
{
	float smoothness;
	float specularStrength;
} material;



void main()
{
	vec3 lightDir 	= tbnMatrix * -light.direction.xyz;
	vec3 normal 	= texture(texSampler[NORMAL_INDEX], fragTexCoord).xyz * 2.0 - 1.0;
	normal 			= normalize(normal);

	float ldotn = max(0, dot(lightDir, normal));

	vec4 tex = texture(texSampler[ALBEDO_INDEX], fragTexCoord);
	vec3 albedo = tex.rgb;

	float lightIntensity = ldotn;


	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);// * 0.8;
	float inLightActual = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);

	// SHADOWS
	// lightIntensity = min(lightIntensity, inLight);	
	lightIntensity = lightIntensity * inLight;
	lightIntensity = inLight;


	vec3 worldSpaceNormal = inverse(tbnMatrix) * normal;
	// vec3 worldSpaceNormal = inverse(tbnMatrix) * vec3(0,0,1);

	SunAndAmbientLights lights = BETTER_compute_sky_color(worldSpaceNormal);

	// outColor.rgb = lights.ambient;
	// outColor.a = 1;
	// return;

	vec3 lightColor = compute_sky_color(worldSpaceNormal);


	/// SPECULAR
	float smoothness 		= material.smoothness;
	float specularStrength 	= 0.1;//material.specularStrength;

	vec3 viewDirection = normalize(tbnMatrix * (light.cameraPosition.xyz - fragPosition));
	vec3 halfVector = normalize(lightDir + viewDirection);
	float spec = max(0, dot (halfVector, normal));
	spec = pow(spec, 256 * smoothness);
	vec3 specularTerm = spec * specularStrength * lightColor * inLightActual;

	/// -------------------------------------------------

	vec3 diffuseTerm = lightColor * albedo * lightIntensity;


	// vec3 ambientColor 		= compute_sky_color(normal, tbnMatrix);
	vec3 ambientColor 		= lightColor; //compute_sky_color(worldSpaceNormal);
	// float ambientIntensity 	= 0.3;
	// vec3 ambientTerm 		= ambientIntensity * ambientColor * albedo;
	vec3 ambientTerm = lights.ambient * albedo;
	diffuseTerm = lights.sun * albedo * inLight;


	// outColor.rgb 	= diffuseTerm + specularTerm + ambientTerm;
	outColor.rgb 	= diffuseTerm + ambientTerm;
	outColor.a 		= tex.a;
}