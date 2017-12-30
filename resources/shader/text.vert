#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec2 texcoord;
uniform mat4 matrix;

out vec2 coord2d;

void main()
{
	coord2d=texcoord;
	gl_Position=matrix*position;
}
