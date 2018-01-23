#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec3 coord;
layout(location=2) in float face;
layout(location=3) in vec3 lighting;

uniform mat4 matrix;
uniform float dayLight;

//x: intensity
//y: ao
//z: light
out vec3 frag_lighting;
out vec3 frag_texcoord;
out vec3 frag_pos;

const float AOcurve[4] = float[4](0.54, 0.7569, 0.87, 1.0);
const float SunLightCurve[16] = float[16](
		0.000000, 0.022283, 0.047749, 0.076854, 
		0.110116, 0.148129, 0.191574, 0.241224, 
		0.297968, 0.362818, 0.436932, 0.521634, 
		0.618436, 0.729067, 0.855502, 1.000000);
const float TorchLightCurve[16] = float[16](
		0.000000, 0.033424, 0.071624, 0.115280, 
		0.165173, 0.222194, 0.287361, 0.361837, 
		0.446952, 0.544227, 0.655398, 0.782451, 
		0.927654, 1.093600, 1.283254, 1.500000);
const float intensities[6] = float[6](0.8, 0.8, 1.0, 0.7, 0.9, 0.9);

void main()
{
	frag_lighting.x = intensities[int(face + 0.5f)];
	frag_lighting.y = AOcurve[int(lighting.x + 0.5f)];
	float sunLight = SunLightCurve[int(lighting.y + 0.5f)] * dayLight + 0.1f;
	float torchLight = TorchLightCurve[int(lighting.z + 0.5f)];
	frag_lighting.z = max(sunLight, torchLight);

	gl_Position = matrix * position;

	frag_pos = position.xyz;
	frag_texcoord = coord;
}
