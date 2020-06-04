#version 450

layout (set = 3, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 color;
	vec4 ambient;
} light;

// layout(location = 0) in vec3 fragColour;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragPosition;
layout(location = 4) in vec4 lightCoords;
// layout(location = 5) in mat3 tbnMatrix;

layout(location = 0) out vec4 outColor;

layout(binding = 0, set = 4) uniform sampler2D lightMap;

const vec3 putple = vec3(0.62, 0.3, 0.8);
const vec3 bluish = vec3(0.3, 0.62, 0.8);
const vec3 green = vec3(0.52, 0.7, 0.40);

void main()
{
	vec3 lightDir = -light.direction.xyz;
	vec3 normal = fragNormal;

	// Todo(Leo): Add translucency
	float ldotn = max(0, dot(lightDir, normal));

	float lightIntensity = ldotn;

	vec3 albedo = green;

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
	// color *= step(distanceFromCenter, 1.0);
	outColor.rgb = color;

	float opacity = step (distanceFromCenter, 1.0);
	opacity *= (step(fragTexCoord.y, 0.0) + step(0.2, fragTexCoord.x + fragTexCoord.y));

	if (opacity < 0.5)
	{
		discard;
	}

	outColor.a 					= 1;
}