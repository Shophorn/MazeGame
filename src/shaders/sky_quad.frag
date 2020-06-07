#version 450

layout (location = 0) in vec3 normal;

layout (location = 0) out vec4 outColor;

void main()
{
	// vec3 light = -normalize(vec3(1, -1.2, -3));
	float d = dot(normal, vec3(0,0,1));
	float d2 = dot(normal, vec3(0,1,0));
	float d3 = dot(normal, vec3(1,0,0));
// normalize_v3({1, -1.2, -3}

	if (d > 0)
	{
		// outColor.rgb = mix(vec3(), vec3(), d);
		outColor.rgb = vec3(1,1,1);
	}
	else
	{
		outColor.rgb = vec3(0,0,0.9);
	}

	if (d2 > 0)
	{
		outColor.r = 1;
	}
	else
	{
		outColor.b = 0;
	}

	if (d3 > 0)
	{
		outColor.g = 1;
	}

	outColor.rgb = normal / 2 + 0.5;

	outColor.rgb = floor(outColor.rgb * 2);

	// if (d > 0.9)
	// {
	// 	outColor.rgb = vec3(0,0,0);
	// }

	// outColor.rgb = (normalize(normal) + 1) / 2;
	// outColor.rgb = outColor.bbb;
	// // outColor.rgb = normal;
	outColor.a = 1;
}