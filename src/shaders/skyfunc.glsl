vec3 compute_sky_color(vec3 normal)
{
	vec3 light = -normalize(lightInput.direction.xyz);

	float d = dot(light, normal);

	d = (d + 1) / 2;
	vec3 color = mix(	texture(texSampler[0], vec2(max(0, d), 0)).rgb,
						texture(texSampler[1], vec2(max(0, d), 0)).rgb,
						lightInput.skyColourSelection);

	return color;
}
