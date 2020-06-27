#version 450

layout(location = 1) in vec2 fragTexCoord;

void main()
{
	float distanceFromCenter 	= length(fragTexCoord.xy * 2);
	float opacity 				= step (distanceFromCenter, 1.0);

	const float angle = 0.2;

	float x = fragTexCoord.x * 2;
	float y = fragTexCoord.y * 2;

	float op2 = angle * y + angle;
	opacity *= step(abs(op2), abs(x));

	if (opacity < 0.5)
	{
		discard;
	}



	if (opacity < 0.5)
	{
		discard;
	}
}