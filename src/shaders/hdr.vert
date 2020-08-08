#version 450

const vec2 vertexPositions [] =
{
	vec2 (-1, -1),
	vec2 (3, -1),
	vec2 (-1, 3)	
};

layout(location = 0) out vec2 fragTexCoord;

void main()
{
	gl_Position = vec4(vertexPositions[gl_VertexIndex], 0, 1);
	fragTexCoord = (vertexPositions[gl_VertexIndex] + 1) / 2;
}