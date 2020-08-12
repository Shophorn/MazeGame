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
