#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec2 coord;
uniform mat4 matrix;

out vec2 texcoord;

void main()
{
	gl_Position = matrix * position;
	texcoord = coord;
}
