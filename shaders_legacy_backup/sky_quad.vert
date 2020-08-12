#version 450

layout(push_constant) uniform Things
{
	mat4 rotationMatrix;
} things;

layout (set = 0, binding = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
	mat4 lightViewProjection;
	float shadowDistance;
	float shadowTransitionDistance;
} camera;

const vec4 vertexPositions[4] = 
{
	// Note(Leo): These seen inverted

	vec4 (-1, 1, 0, 1),
	vec4 ( 1, 1, 0, 1),
	vec4 (-1,-1, 0, 1),
	vec4 ( 1,-1, 0, 1),
};

const vec3 debugColors[4] =
{
	vec3(1,0,0),
	vec3(1,1,0),
	vec3(0,1,0),
	vec3(0,1,1),
};

layout (location = 0) out vec3 normal;

void main()
{
	const float halfVerticalFov = radians(25); // degrees
	const float halfHorizontalFov = halfVerticalFov;// * (960 / 540);



	gl_Position = vertexPositions[gl_VertexIndex];

	float cosH = cos(halfHorizontalFov);
	float sinH = sin(halfHorizontalFov);

	float cosV = cos(halfVerticalFov);
	float sinV = sin(halfVerticalFov);

	// Note(Leo): gl_Position gives signs only
	// Note(Leo): y is inverted in above vertexPositions
	normal = vec3 (	sinH * gl_Position.x,
	 				cosH * cosV,
	 				sinV * -gl_Position.y);


	normal = ((things.rotationMatrix) * vec4(normal, 0)).xyz;

	// normal = things.rotationMatrix[0];

	// normal = debugColors[gl_VertexIndex];

	// normal = vec3(0.8,0,0.1);
	// normal = things.cameraDirection;

	// Note(Leo): "Plane" is "vertical"
	// normal.x = sin(halfHorizontalFov) * gl_Position.x;
	// normal.y = cos(halfVerticalFov) * cos(halfHorizontalFov);
	// normal.z = sin(halfVerticalFov) * gl_Position.y;
	// normal = - normalize(normal);




	// normal = (camera.view * vec4(normal, 0)).x	yz;
	// normal = (transposinverse(camera.view) * vec4(normal, 0)).xyz;
	// float x = sin(halfHorizontalFov) * gl_Position.x;
	// float y = sin(halfVerticalFov) * gl_Position.y;

	// vec3 fakeWorldPosition = vec3(x, y, cos(halfVerticalFov) * cos(halfVerticalFov));
	// normal = normalize(fakeWorldPosition);





	// normal = (vertexPositions[gl_VertexIndex].xyz + 1) / 2;
}