#version 450

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoord;

layout (push_constant) uniform GuiInfo
{
	vec2 quad[4];
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
	gl_Position 	= vec4(gui.quad[gl_VertexIndex], 0, 1);
	fragColor 		= gui.color.rgb;
	fragTexCoord 	= texcoords[gl_VertexIndex];
}

