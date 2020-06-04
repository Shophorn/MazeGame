// #version 450

// layout(location = 1) in vec2 fragTexCoord;

// void main()
// {
// 	float distanceFromCenter 	= length(fragTexCoord.xy * 2);

// 	float opacity = step (distanceFromCenter, 1.0);
// 	opacity *= (step(fragTexCoord.y, 0.0) + step(0.2, fragTexCoord.x + fragTexCoord.y));

// 	if (opacity < 0.5)
// 	{
// 		gl_FragDepth = 0;
// 		discard;
// 	}
// }