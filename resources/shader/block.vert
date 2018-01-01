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
0.146974, 0.167016, 0.189791, 0.215671,
0.245081, 0.278501, 0.316478, 0.359635,
0.408676, 0.464404, 0.527732, 0.599695,
0.681472, 0.774400, 0.880000, 1.000000);

void main()
{
	frag_face=face;
	frag_ao = AOcurve[int(lighting.x+0.5)];
	frag_light = LIcurve[int(lighting.y+0.5)];

	gl_Position = matrix*position;

   	pos = position.xyz;
   	texcoord = coord;
}
