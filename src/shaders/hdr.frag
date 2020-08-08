#version 450

// Todo(Leo): not actully hdr yet
layout (binding = 0, set = 0) uniform sampler2D hdrTexture;

layout (location = 0) in vec2 fragTexCoord;
layout (location = 0) out vec4 outColor;


void main()
{
	outColor.rgb = texture(hdrTexture, fragTexCoord).rgb;
	if (fragTexCoord.x < 0.5)
	{
		outColor.rgb = vec3(1,1,1) - outColor.rgb;
	}
	outColor.a = 1;
}