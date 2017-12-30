#version 330 core
uniform sampler2D sampler;
uniform vec4 bgColor;
uniform vec4 textColor;
in vec2 coord2d;

out vec4 color;

void main()
{
	vec4 c=texture(sampler,coord2d);
	if(coord2d.y<=0.05)
		color=bgColor;
	else
		color=mix(bgColor, textColor, c.z+0.3);
}
