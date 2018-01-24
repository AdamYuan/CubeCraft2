#version 330 core
layout(location=0) in vec4 position;
uniform mat4 matrix;

out float Y;

void main()
{
	Y = 0.5f - position.y / 2.0f;
	gl_Position = matrix * position;
}
