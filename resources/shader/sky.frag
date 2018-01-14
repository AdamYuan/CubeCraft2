#version 330 core
uniform float Time;
uniform sampler2D sampler;

in float Y;
out vec4 color;
void main()
{
	color = texture(sampler, vec2(Time, Y));
}
