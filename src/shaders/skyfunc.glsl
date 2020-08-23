layout(set = SKY_GRADIENT_SET, binding = 0) uniform sampler2D skyGradients[2];

layout (set = LIGHTING_SET, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 IGNORED_color;
	vec4 ambient;
	vec4 cameraPosition;
	float skyColourSelection;
} light;

vec3 actual_compute_sky_color(float dotHorizon, float dotLight)
{

	// -------------------------------------------------------------



	const float horizonClip = step(0, dotHorizon);

	// -------------------------------------------------------------

	vec3 skyTop 	= vec3(0.05, 0.2, 0.7);
	vec3 skyBottom 	= vec3(0.1, 0.3, 0.9);

	vec3 skyGradientColor = mix(skyBottom, skyTop, dotHorizon);




	// -------------------------------------------------------------
	float horizonHaloFalloff 	= 0.2;
	// float horizonHaloDistanceFalloff = ???;
	// float horizonHaloIntensityFalloff = ???;

	vec3 horizonHaloColor 	= vec3(0.9, 0.9, 0.8);
	horizonHaloColor 		*= exp(-dotHorizon / horizonHaloFalloff);



	// -------------------------------------------------------------

	vec3 sunDiscColor 	= vec3(10,10,5);
	sunDiscColor 		*= smoothstep(0.9985, 0.9992, dotLight);

	// -------------------------------------------------

	float sunHaloFalloff = 0.03;
	vec3 sunHaloColor 	= vec3(3,3,1);
	sunHaloColor 		*= exp(-(1 - dotLight) / sunHaloFalloff);

	// -------------------------------------------------


	vec3 finalColor = vec3(0,0,0);
	finalColor += skyGradientColor;
	finalColor += horizonHaloColor;
	finalColor += sunDiscColor;
	finalColor += sunHaloColor;
	finalColor *= horizonClip;

	return finalColor;


/*
	vec2 uv = vec2(d, 0);

	vec3 colorA = texture(skyGradients[0], uv).rgb;
	vec3 colorB = texture(skyGradients[1], uv).rgb;

	vec3 color = mix(colorA, colorB, light.skyColourSelection);
	return color;
*/	
}


// For actual skybox
vec3 compute_sky_color(vec3 normal)
{	
	normal = normalize(normal);


	vec3 lightDir = normalize(-light.direction.xyz);
	float dotLight = dot(lightDir, normal);

	const vec3 up 			= vec3(0,0,1);
	const float dotHorizon 	= dot(up, normal);

	return actual_compute_sky_color(dotHorizon, dotLight);

}


vec3 compute_sky_color(vec3 normal, vec3 lightDir)
{
	float d = dot(lightDir, normal);
	
	d = (d + 1) / 2;
	d = max(0, d);

	vec2 uv = vec2(d, 0);

	vec3 colorA = texture(skyGradients[0], uv).rgb;
	vec3 colorB = texture(skyGradients[1], uv).rgb;

	vec3 color = mix(colorA, colorB, light.skyColourSelection);
	return color;
}


vec3 compute_sky_color(vec3 normal, mat3 tbnMatrix)
{
	vec3 lightDir = tbnMatrix * -light.direction.xyz;

	// vec3 up = tbnMatrix * vec3(0,0,1);
	// float dotHorizon = dot(up, normal);

	// float dotLight = dot(lightDir, normal);

	// return actual_compute_sky_color(dotHorizon, dotLight);


	float d = dot(lightDir, normal);

	d = (d + 1) / 2;
	d = max(0, d);

	vec2 uv = vec2(d, 0);

	vec3 colorA = texture(skyGradients[0], uv).rgb;
	vec3 colorB = texture(skyGradients[1], uv).rgb;

	vec3 color = mix(colorA, colorB, light.skyColourSelection);
	return color;
}