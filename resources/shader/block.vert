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
		0.200000, 0.253333, 0.306667, 0.360000, 
		0.413333, 0.466667, 0.520000, 0.573333, 
		0.626667, 0.680000, 0.733333, 0.786667, 
		0.840000, 0.893333, 0.946667, 1.000000);

void main()
{
	frag_face = face;
	frag_ao = AOcurve[int(lighting.x+0.5)];
	frag_light = LIcurve[int(lighting.y+0.5)];

	gl_Position = matrix * position;

	pos = position.xyz;
	texcoord = coord;
}
