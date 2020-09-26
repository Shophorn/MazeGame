layout(set = SKY_GRADIENT_SET, binding = 0) uniform sampler2D skyGradients[2];

layout (set = LIGHTING_SET, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 IGNORED_color;
	vec4 ambient;
	vec4 cameraPosition;
	
	vec4 skyBottomColor;
	vec4 skyTopColor;
	vec4 skyGroundColor;

	vec4 horizonHaloColorAndFalloff;
	vec4 sunHaloColorAndFalloff;

	vec4 sunDiscColor;
	vec4 sunDiscSizeAndFalloff;

	float IGNORED_skyColourSelection;
} light;

vec3 actual_compute_sky_color(const float dotHorizon, const float dotLight)
{
	// return 	texture(skyGradients[0], vec2(dotHorizon, 0)).rgb 
	// 		+ texture(skyGradients[1], vec2((1-dotLight), 0)).rgb;

	// -------------------------------------------------------------

	float t 	= dotHorizon;
	t = pow(abs(t), 0.2) * sign(t);

	// const vec3 skyGradientColor = 	clamp(-t, 0, 1) * light.skyGroundColor.rgb
	// 								+ (1 - abs(t)) * light.skyBottomColor.rgb
	// 								+ clamp(t, 0, 1) * light.skyTopColor.rgb;



	// float bias 	= 0.5;//light.skyColourSelection;

	// t = mix(mix(0,bias,t), mix(bias,1,t), t);
	// t = mix(mix(0,bias,t), mix(bias,1,t), t);

	float horizonClip 	= step(0, dotHorizon);
	// finalColor 			*= horizonClip;

	const vec3 upperSky = mix(	light.skyBottomColor.rgb,
										light.skyTopColor.rgb,
										t);

	const vec3 skyGradientColor = mix(light.skyGroundColor.rgb, upperSky, horizonClip);




	// -------------------------------------------------------------

	float n 					= 1 - abs(dotHorizon) / light.horizonHaloColorAndFalloff.a;
	float horizonHaloFalloff 	= n*n*n;
	const vec3 horizonHaloColor = max(vec3(0,0,0), light.horizonHaloColorAndFalloff.rgb * horizonHaloFalloff);

	// -------------------------------------------------------------

	vec3 sunDiscColor 		= light.sunDiscColor.rgb;
	float sunDiscSize 		= light.sunDiscSizeAndFalloff.x;
	float sunDiscFalloff 	= light.sunDiscSizeAndFalloff.y * sunDiscSize;
	float halfFalloff 		= sunDiscFalloff * 0.5;

	// sunDiscColor 		*= smoothstep(0.9985, 0.9992, dotLight);
	sunDiscColor 		*= smoothstep((1 - sunDiscSize) - halfFalloff, (1 - sunDiscSize) + halfFalloff, dotLight);

	// -------------------------------------------------

	float d  				= 1 - dotLight;
	float m 				= 1 - d / light.sunHaloColorAndFalloff.a;
	float sunHaloFalloff 	= m*m*m;//pow(m,light.skyColourSelection * 10) * step(d, m);
	const vec3 sunHaloColor = max(vec3(0,0,0), light.sunHaloColorAndFalloff.rgb * sunHaloFalloff);

	// -------------------------------------------------


	vec3 finalColor = vec3(0,0,0);
	finalColor += skyGradientColor;
	finalColor += horizonHaloColor;
	finalColor += sunDiscColor;
	finalColor += sunHaloColor;

	return finalColor;	
}

// For actual skybox and most items
vec3 compute_sky_color(vec3 normal)
{	
	normal = normalize(normal);

	// return normal;

	vec3 lightDir = normalize(-light.direction.xyz);
	float dotLight = dot(lightDir, normal);

	const vec3 up 			= vec3(0,0,1);
	const float dotHorizon 	= dot(up, normal);

	float value = (dotHorizon + dotLight) / 2;

	// return vec3(value, value, value);

	return actual_compute_sky_color(dotHorizon, dotLight);

}

struct SunAndAmbientLights
{
	vec3 sun;
	vec3 ambient;
};

SunAndAmbientLights BETTER_compute_sky_color(vec3 normal)
{
	normal = normalize(normal);

	// return normal;

	vec3 lightDir = normalize(-light.direction.xyz);
	float dotLight = dot(lightDir, normal);

	const vec3 up 			= vec3(0,0,1);
	const float dotHorizon 	= dot(up, normal);



		// return 	texture(skyGradients[0], vec2(dotHorizon, 0)).rgb 
	// 		+ texture(skyGradients[1], vec2((1-dotLight), 0)).rgb;

	// -------------------------------------------------------------

	float t 	= dotHorizon;
	t = pow(abs(t), 0.2) * sign(t);

	// const vec3 skyGradientColor = 	clamp(-t, 0, 1) * light.skyGroundColor.rgb
	// 								+ (1 - abs(t)) * light.skyBottomColor.rgb
	// 								+ clamp(t, 0, 1) * light.skyTopColor.rgb;



	// float bias 	= 0.5;//light.skyColourSelection;

	// t = mix(mix(0,bias,t), mix(bias,1,t), t);
	// t = mix(mix(0,bias,t), mix(bias,1,t), t);

	float horizonClip 	= step(0, dotHorizon);
	// finalColor 			*= horizonClip;

	const vec3 upperSky = mix(	light.skyBottomColor.rgb,
										light.skyTopColor.rgb,
										t);

	const vec3 skyGradientColor = mix(light.skyGroundColor.rgb * light.skyTopColor.rgb, upperSky, horizonClip);




	// -------------------------------------------------------------

	float n 					= 1 - abs(dotHorizon) / light.horizonHaloColorAndFalloff.a;
	float horizonHaloFalloff 	= n*n*n;
	const vec3 horizonHaloColor = max(vec3(0,0,0), light.horizonHaloColorAndFalloff.rgb * horizonHaloFalloff);

	// -------------------------------------------------------------

	vec3 sunDiscColor 		= light.sunDiscColor.rgb;
	float sunDiscSize 		= light.sunDiscSizeAndFalloff.x;
	float sunDiscFalloff 	= light.sunDiscSizeAndFalloff.y * sunDiscSize;
	float halfFalloff 		= sunDiscFalloff * 0.5;

	// sunDiscColor 		*= smoothstep(0.9985, 0.9992, dotLight);
	sunDiscColor 		*= smoothstep((1 - sunDiscSize) - halfFalloff, (1 - sunDiscSize) + halfFalloff, dotLight);

	// -------------------------------------------------

	float d  				= 1 - dotLight;
	float m 				= 1 - d / light.sunHaloColorAndFalloff.a;
	float sunHaloFalloff 	= m*m*m;//pow(m,light.skyColourSelection * 10) * step(d, m);
	const vec3 sunHaloColor = max(vec3(0,0,0), light.sunHaloColorAndFalloff.rgb * sunHaloFalloff);

	// -------------------------------------------------

	SunAndAmbientLights result;
	result.ambient = light.	skyBottomColor.rgb;//skyGradientColor;// + horizonHaloColor;


	result.sun = max(0, dotLight) * light.sunDiscColor.rgb;

	return result;

}
