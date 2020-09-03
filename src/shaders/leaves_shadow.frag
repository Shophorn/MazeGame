#version 450

layout(location = 1) 			in vec2 fragTexCoord;
layout(binding = 0, set = 1) 	uniform sampler2D maskTexture;

void main()
{
	float opacity = texture(maskTexture, fragTexCoord).r;
	if (opacity < 0.5)
	{
		discard;
	}
}