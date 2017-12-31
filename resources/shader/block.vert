#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec3 coord;
layout(location=2) in float face;
layout(location=3) in vec3 lighting;
uniform mat4 matrix;
out float frag_face;
out float frag_ao;
out float frag_light;
out vec3 texcoord;
out vec3 pos;

const float AOcurve[4] = float[4](0.54, 0.7569, 0.87, 1.0);
const float LIcurve[16] = float[16](
0.087354, 0.102770, 0.120905, 0.142242,
0.167343, 0.196874, 0.231617, 0.272491,
0.320577, 0.377150, 0.443705, 0.522006,
0.614125, 0.722500, 0.850000, 1.000000);

void main()
{
	frag_face=face;
	frag_ao = AOcurve[int(lighting.x+0.5)];
	frag_light = LIcurve[int(lighting.y+0.5)];

	gl_Position = matrix*position;

   	pos = position.xyz;
   	texcoord = coord;
}
