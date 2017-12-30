#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec3 coord;
layout(location=2) in float face;
layout(location=3) in vec2 lighting;
uniform mat4 matrix;
out float frag_face;
out float frag_ao;
out float frag_light;
out vec3 texcoord;
out vec3 pos;

const float AOcurve[4] = float[4](0.54, 0.7569, 0.87, 1.0);

void main()
{
	frag_face=face;
	frag_ao = AOcurve[int(lighting.x+0.5)];
	frag_light = ((lighting.y+2.0)/17.0);

	gl_Position = matrix*position;

   	pos = position.xyz;
   	texcoord = coord;
}
