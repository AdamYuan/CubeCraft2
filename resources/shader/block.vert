#version 330 core
layout(location=0) in vec4 position;
layout(location=1) in vec3 coord;
layout(location=2) in float face;
layout(location=3) in vec3 lighting;

uniform mat4 matrix;
uniform float dayLight;
uniform vec3 camera;

//x: intensity
//y: ao
//z: light
out vec3 frag_lighting;
out vec3 frag_texcoord;
out vec3 frag_pos;
out float fog_height;

const float AOcurve[4] = float[4](0.54, 0.7569, 0.87, 1.0);
const float SunLightCurve[16] = float[16](
		0.000000, 0.066667, 0.133333, 0.200000, 
		0.266667, 0.333333, 0.400000, 0.466667, 
		0.533333, 0.600000, 0.666667, 0.733333, 
		0.800000, 0.866667, 0.933333, 1.000000);
const float TorchLightCurve[16] = float[16](
		0.000000, 0.100000, 0.200000, 0.300000, 
		0.400000, 0.500000, 0.600000, 0.700000, 
		0.800000, 0.900000, 1.000000, 1.100000, 
		1.200000, 1.300000, 1.400000, 1.500000);
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

	//fog
	float dx = distance(position.xz, camera.xz);
	float dy = position.y - camera.y;
	fog_height = 0.5f - (dy / sqrt(dx*dx + dy*dy)) / 2.0f;
}
