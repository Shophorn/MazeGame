#version 450

layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec2 fragTexCoord;

layout (push_constant) uniform GuiInfo
{
	// vec2 quad[4];
	vec2 position;
	vec2 size;
	vec2 uvPosition;
	vec2 uvSize;

	// Todo(Leo): move to fragment shader
	vec4 color;
} gui;

const vec2 texcoords [4] =
{
	vec2(0,0),
	vec2(1,0),
	vec2(0,1),
	vec2(1,1),
};

void main ()
{
	float u = (gl_VertexIndex % 2);
	float v = (gl_VertexIndex / 2);

	gl_Position = vec4(	gui.position.x + u * gui.size.x,
						gui.position.y + v * gui.size.y,
						0, 1);

	// gl_Position 	= vec4(gui.quad[gl_VertexIndex], 0, 1);
	fragColor 		= gui.color;
	fragTexCoord 	= vec2 (gui.uvPosition.x + u * gui.uvSize.x,
							gui.uvPosition.y + v * gui.uvSize.y);
	// fragColor 		= vec3(fragTexCoord, 0);
}

