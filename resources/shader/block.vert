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
		0.134934, 0.154210, 0.176240, 0.201417, 
		0.230191, 0.263076, 0.300658, 0.343609, 
		0.392696, 0.448795, 0.512909, 0.586182, 
		0.669922, 0.765625, 0.875000, 1.000000);
const float intensities[6] = float[6](0.7, 0.7, 1.0, 0.6, 0.85, 0.85);

void main()
{
	frag_lighting.x = intensities[int(face + 0.5f)];
	frag_lighting.y = AOcurve[int(lighting.x + 0.5f)];
	frag_lighting.z = LIcurve[min(int(max(lighting.y, lighting.z + 1.0f) + 0.5f), 15)];

	gl_Position = matrix * position;

	frag_pos = position.xyz;
	frag_texcoord = coord;
}
