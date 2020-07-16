#version 450

layout (set = 1, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 color;
	vec4 ambient;
} light;

layout(binding = 0, set = 2) uniform sampler2D lightMap;

layout(push_constant) uniform Block
{
	int colourIndex;
} colourIndex;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 lightCoords;

layout(location = 0) out vec4 outColor;


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

	float distanceFromCenter 	= length(fragTexCoord.xy * 2);
	float opacity 				= step (distanceFromCenter, 1.0);

	const float angle = 0.2;

	float x = fragTexCoord.x * 2;
	float y = fragTexCoord.y * 2;

	float op2 = angle * y + angle;
	opacity *= step(abs(op2), abs(x));

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
	vec3 albedo = green;

	// albedo = mix(vec3(0,0,0), vec3(0,1,1), fragTexCoord.y + 0.5);
	// albedo = mix(darkerGreen, lighterGreen, fragTexCoord.y + 0.5);
	albedo = mix(	darkerColours[colourIndex.colourIndex],
					lighterColours[colourIndex.colourIndex],
					fragTexCoord.y + 0.5);
	albedo = pow(albedo, vec3(gamma));

	float lightIntensity = ldotn;


	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float shadowBias = 0.001;
	float inLight = 1.0 - step(lightDepthFromTexture + shadowBias, lightCoords.z);// * 0.8;

	// SHADOWS
	lightIntensity = lightIntensity * inLight;	

	// Note(Leo): try these two
	vec3 lightColor = mix(light.ambient.rgb, light.color.rgb, lightIntensity);
	// vec3 lightColor = light.ambient.rgb + light.color.rgb * lightIntensity;
	
	vec3 color = lightColor * albedo;

	// color *= step(distanceFromCenter, 1.0);
	outColor.rgb = color;

	outColor.rgb = pow(outColor.rgb, vec3(1/gamma));


	outColor.a 					= 1;
}