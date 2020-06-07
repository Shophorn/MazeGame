#version 450

layout(location = 1) in vec3 fragTexCoord;

layout(location = 0) out vec4 outColor;

const int SAMPLER_BIND_ID = 0;
const int SAMPLER_SET_ID = 1;

layout(	binding = SAMPLER_BIND_ID, set = SAMPLER_SET_ID) uniform samplerCube texSampler;

layout (set = 3, binding = 0) uniform Lighting
{
	vec4 direction;
	vec4 color;
	vec4 ambient;
} light;

vec3 evaluateSkyLight (float dot)
{
	const uint gradientCount = 6;
	const vec3 gradientValues [gradientCount] =
	{
		vec3(0.43 / 2.55, 0.3/2.55, 0.98/2.55),
		vec3(0.43 / 2.55, 0.3/2.55, 0.98/2.55),
		vec3(0.4, 0.9, 1.0),
		vec3(0.8, 0.99, 1.0),
		vec3(1,1,1),
		vec3(1,1,1),
	};

	const float gradientPositions [gradientCount] =
	{
		0.0,
	 	0.1,
	 	0.3,
	 	0.99,
	 	0.999,
	 	1.0,
	};

	for(int i = 1; i < gradientCount; ++i)
	{
		if (dot < gradientPositions[i])
		{
			float t = (dot - gradientPositions[i - 1]) / (gradientPositions[i] - gradientPositions[i - 1]);
			return mix(gradientValues[i - 1], gradientValues[i], t);
		}
	}
}

void main()
{
	vec3 light = -normalize(light.direction.xyz);
	vec3 normal = normalize(fragTexCoord);
	float d = dot(light, normal);

	d = (d + 1) / 2;
	vec3 color = evaluateSkyLight(d);

	outColor.rgb 	= color;
	outColor.a 		= 1.0;
}