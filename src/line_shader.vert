#version 450

layout (set = 0) uniform CameraProjections 
{
	mat4 view;
	mat4 projection;
} camera;

layout(push_constant) uniform LineInfo
{
	vec4 start;
	vec4 end;
	vec4 color;
} line;

layout (location = 0) out vec3 fragColor;

void main ()
{
	float mixValue 	= float(gl_VertexIndex);
	vec3 position 	= mix(line.start.xyz, line.end.xyz, mixValue);

	gl_Position 	= camera.projection * camera.view * vec4(position, 1.0);
	fragColor 		= line.color.rgb;
}

