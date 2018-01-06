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
0.205891, 0.228768, 0.254186, 0.282429,
0.313811, 0.348678, 0.387420, 0.430467,
0.478297, 0.531441, 0.590490, 0.656100,
0.729000, 0.810000, 0.900000, 1.000000);

void main()
{
	frag_face=face;
	frag_ao = AOcurve[int(lighting.x+0.5)];
	frag_light = LIcurve[int(lighting.y+0.5)];

	gl_Position = matrix*position;

   	pos = position.xyz;
   	texcoord = coord;
}
