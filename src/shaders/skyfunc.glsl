layout(set = SKY_GRADIENT_SET, binding = 0) uniform sampler2D skyGradients[2];

layout (set = LIGHTING_SET, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 IGNORED_color;
	vec4 ambient;
	vec4 cameraPosition;
	
	vec4 skyBottomColor;
	vec4 skyTopColor;

	vec4 horizonHaloColorAndFalloff;
	vec4 sunHaloColorAndFalloff;

	float skyColourSelection;
} light;

vec3 actual_compute_sky_color(const float dotHorizon, const float dotLight)
{
	// return 	texture(skyGradients[0], vec2(dotHorizon, 0)).rgb 
	// 		+ texture(skyGradients[1], vec2((1-dotLight), 0)).rgb;



	// -------------------------------------------------------------

	float t 	= dotHorizon;
	float bias 	= light.skyColourSelection;

	t = mix(mix(0,bias,t), mix(bias,1,t), t);
	t = mix(mix(0,bias,t), mix(bias,1,t), t);

	const vec3 skyGradientColor = mix(	light.skyBottomColor.rgb,
										light.skyTopColor.rgb,
										t);


	// -------------------------------------------------------------

	float n 					= 1 - dotHorizon / light.horizonHaloColorAndFalloff.a;
	float horizonHaloFalloff 	= n*n*n;
	const vec3 horizonHaloColor = max(vec3(0,0,0), light.horizonHaloColorAndFalloff.rgb * horizonHaloFalloff);

	// -------------------------------------------------------------

	vec3 sunDiscColor 	= vec3(10,10,5);
	sunDiscColor 		*= smoothstep(0.9985, 0.9992, dotLight);

	// -------------------------------------------------

	float d  				= 1 - dotLight;
	float m 				= 1 - d / light.sunHaloColorAndFalloff.a;
	float sunHaloFalloff 	= pow(m,light.skyColourSelection * 10) * step(d, m);
	const vec3 sunHaloColor = max(vec3(0,0,0), light.sunHaloColorAndFalloff.rgb * sunHaloFalloff);

	// -------------------------------------------------


	vec3 finalColor = vec3(0,0,0);
	finalColor += skyGradientColor;
	finalColor += horizonHaloColor;
	finalColor += sunDiscColor;
	finalColor += sunHaloColor;

	float horizonClip 	= step(0, dotHorizon);
	finalColor 			*= horizonClip;

	return finalColor;	
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