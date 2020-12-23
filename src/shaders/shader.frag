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
const int AO_INDEX 			= 2;
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


	// Note(Leo): lghting projection is orthographic, so no perspective divide
	vec3 lightProjectionCoords 	= lightCoords.xyz;
	// Note(Leo): with vulkan z is already on 0 to 1 range (by default):
	// Todo(Leo): we probably shoud be explicit about it in code
	lightProjectionCoords.xy 	= lightProjectionCoords.xy * 0.5 + 0.5;
	
	float lightDepthFromTexture = texture(lightMap, lightProjectionCoords.xy).r;

	// Todo(Leo): Not const but get from uniform settings
	// Todo(Leo): change this based on normal of surface related to light direction
	const float bias = 0.001;

	float closestDepth = lightDepthFromTexture;// + bias;
	float currentDepth = lightProjectionCoords.z;


	// float shadow = step(currentDepth, closestDepth);
	float shadow = currentDepth - bias > closestDepth ? 1 : 0;

	// outColor = vec4(lightProjectionCoords.xy, 0, 1);


	if (lightProjectionCoords.z > 1.0)
	{
		shadow = 0.0;
	}


	float inLight = 1.0 - shadow; //step(lightDepthFromTexture + epsilon, lightProjectionCoords.z);// * 0.8;


	// SHADOWS
	// lightIntensity = min(lightIntensity, inLight);	
	lightIntensity = lightIntensity * inLight;
	lightIntensity = inLight;


	vec3 worldSpaceNormal = inverse(tbnMatrix) * normal;
	// vec3 worldSpaceNormal = inverse(tbnMatrix) * vec3(0,0,1);

	SunAndAmbientLights lights = BETTER_compute_sky_color(worldSpaceNormal);

	float ambientOcclusion = texture(texSampler[AO_INDEX], fragTexCoord).r;

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
	vec3 specularTerm = spec * specularStrength * lightColor * inLight;

	/// -------------------------------------------------

	vec3 diffuseTerm = lightColor * albedo * lightIntensity;


	// vec3 ambientColor 		= compute_sky_color(normal, tbnMatrix);
	vec3 ambientColor 		= lightColor; //compute_sky_color(worldSpaceNormal);
	// float ambientIntensity 	= 0.3;
	// vec3 ambientTerm 		= ambientIntensity * ambientColor * albedo;
	vec3 ambientTerm = lights.ambient * albedo * ambientOcclusion;
	diffuseTerm = lights.sun * albedo * inLight;


	// outColor.rgb 	= diffuseTerm + specularTerm + ambientTerm;
	outColor.rgb 	= diffuseTerm + ambientTerm;

	if(shadow > 0.5)
	{
		// outColor.rgb = mix(outColor.rgb, vec3(lightCoords.xy, 0), 0.3);

		if (lightProjectionCoords.x < 0) outColor.rgb = vec3(1,0,0);
		if (lightProjectionCoords.x > 1) outColor.rgb = vec3(1,0,1);
		if (lightProjectionCoords.y < 0) outColor.rgb = vec3(0,1,0);
		if (lightProjectionCoords.y > 1) outColor.rgb = vec3(0,1,1);
	}


	outColor.a 		= tex.a;
}