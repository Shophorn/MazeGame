#version 450

// Todo(Leo): not actully hdr yet
layout (binding = 0, set = 0) uniform sampler2D hdrTexture;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 0) out vec4 outColor;

void main()
{	
	vec3 hdrColor = texture(hdrTexture, fragTexCoord).rgb;

	/// TONE MAP
	// vec3 color = hdrColor / (hdrColor + vec3(1.0));
	const float exposure = 0.5;
	vec3 color = vec3(1.0) - exp(-hdrColor * exposure);


	/// GAMMA CORRECTION
	const float gamma = 2.2;
	color = pow(color, vec3(1.0/gamma));

	// /// INVERT
	// if (fragTexCoord.x < 0.5)
	// {
	// 	color.rgb = vec3(1,1,1) - color.rgb;
	// }

	/// OUTPUT
	outColor.rgb 	= color;
	outColor.a 		= 1;
}