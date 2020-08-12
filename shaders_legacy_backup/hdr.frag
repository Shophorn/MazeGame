#version 450

layout (binding = 0, set = 0) uniform sampler2D hdrTexture;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 0) out vec4 outColor;

void main()
{	
	vec3 hdrColor = texture(hdrTexture, fragTexCoord).rgb;

	/// TONE MAP
	vec3 color = hdrColor / (hdrColor + vec3(1.0));
	// const float exposure = 0.5;
	// vec3 color = vec3(1.0) - exp(-hdrColor * exposure);


	/// GAMMA CORRECTION
	const float gamma = 2.2;
	color = pow(color, vec3(1.0/gamma));

	// /// INVERT
	// if (fragTexCoord.x < 0.5)
	// {
	// 	color.rgb = vec3(1,1,1) - color.rgb;
	// }



	/// CONTRAST
	/// https://www.dfstudios.co.uk/articles/programming/image-programming-algorithms/image-processing-algorithms-part-5-contrast-adjustment/
	float contrast = 0.05;
	float factor = (1.01 * (contrast + 1.0)) / (1.0 * (1.01 - contrast));
	color = clamp(factor * (color - 0.5) + 0.5, 0, 1);


	/// OUTPUT
	outColor.rgb 	= color;
	outColor.a 		= 1;
}