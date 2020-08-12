#version 450

layout (location = 1) out vec2 fragTexCoord;

layout (push_constant) uniform GuiInfo
{
	vec2 position;
	vec2 size;
	vec2 uvPosition;
	vec2 uvSize;
} gui;

void main ()
{
	float u = (gl_VertexIndex % 2);
	float v = (gl_VertexIndex / 2);

	gl_Position = vec4(	gui.position.x + u * gui.size.x,
						gui.position.y + v * gui.size.y,
						0, 1);

	fragTexCoord = vec2(gui.uvPosition.x + u * gui.uvSize.x,
						gui.uvPosition.y + v * gui.uvSize.y);
}

