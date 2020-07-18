#version 450

layout(location = 1) in vec2 fragTexCoord;

layout(binding = 0, set = 1) uniform sampler2D maskTexture;

void main()
{
	// float distanceFromCenter 	= length(fragTexCoord.xy * 2);
	// float opacity 				= step (distanceFromCenter, 1.0);

	// const float angle = 0.2;

	// float x = fragTexCoord.x * 2;
	// float y = fragTexCoord.y * 2;

	// float op2 = angle * y + angle;
	// opacity *= step(abs(op2), abs(x));

	float opacity = texture(maskTexture, fragTexCoord).r;

	if (opacity < 0.5)
	{
		discard;
	}
}