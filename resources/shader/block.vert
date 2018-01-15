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
		0.000000, 0.060000, 0.120000, 0.180000, 
		0.240000, 0.300000, 0.360000, 0.420000, 
		0.480000, 0.540000, 0.600000, 0.660000, 
		0.720000, 0.780000, 0.840000, 0.900000);
const float TorchLightCurve[16] = float[16](
		0.000000, 0.083333, 0.166667, 0.250000, 
		0.333333, 0.416667, 0.500000, 0.583333, 
		0.666667, 0.750000, 0.833333, 0.916667, 
		1.000000, 1.083333, 1.166667, 1.250000);
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
