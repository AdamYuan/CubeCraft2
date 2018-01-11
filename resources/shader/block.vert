#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec3 coord;
layout(location=2) in float face;
layout(location=3) in vec3 lighting;
uniform mat4 matrix;

//x: intensity
//y: ao
//z: sunlight
out vec3 frag_lighting;

out vec3 frag_texcoord;
out vec3 frag_pos;

const float AOcurve[4] = float[4](0.54, 0.7569, 0.87, 1.0);
const float LIcurve[16] = float[16](
		0.205891, 0.228768, 0.254186, 0.282429, 
		0.313811, 0.348678, 0.387420, 0.430467, 
		0.478297, 0.531441, 0.590490, 0.656100, 
		0.729000, 0.810000, 0.900000, 1.000000);
const float intensities[6] = float[6](0.7, 0.7, 1.0, 0.6, 0.85, 0.85);

void main()
{
	frag_lighting.x = intensities[int(face+0.5f)];
	frag_lighting.y = AOcurve[int(lighting.x+0.5f)];
	frag_lighting.z = LIcurve[int(lighting.y+0.5f)];

	gl_Position = matrix * position;

	frag_pos = position.xyz;
	frag_texcoord = coord;
}
