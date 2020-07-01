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
	vec4 cameraPosition;
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


layout(set = 1, binding = 0) uniform sampler2D texSampler[TEXTURE_COUNT];
layout(set = 4, binding = 0) uniform sampler2D lightMap;

layout (push_constant) uniform MaterialBlock
{
	float smoothness;
	float specularStrength;
} material;


void main()
{
	vec3 lightDir = tbnMatrix * -light.direction.xyz;
	vec3 normal = texture(texSampler[NORMAL_INDEX], fragTexCoord).xyz * 2.0 - 1.0;
	normal = normalize(normal);

	float ldotn = max(0, dot(lightDir, normal));


	float gamma = 2.2;
	vec4 tex = texture(texSampler[ALBEDO_INDEX], fragTexCoord);
	vec3 albedo = tex.rgb;
	albedo = pow(albedo, vec3(gamma));	
#if 0
	vec3 ambient 	= light.ambient.rgb;
	vec3 diffuse 	= ldotn * light.color.rgb;


	vec3 viewDirection = normalize(light.cameraPosition.xyz - fragPosition);
	viewDirection = tbnMatrix * viewDirection;

	// Note(Leo): invert lightdir so that it points on surface and is reflected properly
	vec3 reflectDirection = reflect(-lightDir, normal);

	float spec = pow(max(0, dot(viewDirection, reflectDirection)), 128);
	spec *= 0.5;
	vec3 specular = spec * light.color.rgb;

	vec3 totalLight = ambient + diffuse + spec;



	outColor.rgb = totalLight * albedo;
	outColor.a = 1;

	return;
#endif

	float lightIntensity = ldotn;


	float lightDepthFromTexture = texture(lightMap, lightCoords.xy).r;

	const float epsilon = 0.0001;
	float inLight = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);// * 0.8;
	float inLightActual = 1.0 - step(lightDepthFromTexture + epsilon, lightCoords.z);

	// SHADOWS
	// lightIntensity = min(lightIntensity, inLight);	
	lightIntensity = lightIntensity * inLight;

	/// SPECULAR
	float smoothness 		= material.smoothness;
	float specularStrength 	= material.specularStrength;

	vec3 viewDirection = normalize(tbnMatrix * (light.cameraPosition.xyz - fragPosition));

#if 0
	vec3 reflectDirection = reflect(-lightDir, normal);
	float spec = max(0, dot (viewDirection, reflectDirection));
#endif

	vec3 halfVector = normalize(lightDir + viewDirection);
	float spec = max(0, dot (halfVector, normal));

	spec = pow(spec, 256 * smoothness);

	vec3 specularTerm = spec * specularStrength * light.color.rgb * inLightActual;

	// lightIntensity *= 2;
	vec3 lightColor = mix(light.ambient.rgb, light.color.rgb, lightIntensity);
	// vec3 lightColor = light.ambient.rgb + light.color.rgb * lightIntensity;
	
	vec3 color = lightColor * albedo + specularTerm;

	outColor.rgb = color;
	outColor.a = tex.a;


	outColor.rgb = pow(outColor.rgb, vec3(1/gamma));
}