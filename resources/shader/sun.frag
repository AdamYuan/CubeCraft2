#version 330 core
uniform sampler2D sampler;

in vec2 texcoord;
out vec4 color;
void main()
{
	color = texture(sampler, texcoord);
    color.a = (color.r + color.g + color.b) / 3.0f;
    if(color.a == 0.0f)
        discard;
}
