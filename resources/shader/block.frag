#version 330 core
in vec3 pos;
in vec3 texcoord;
in float frag_face;
in float frag_ao;
in float frag_light;
out vec4 color;
uniform sampler2DArray sampler;
uniform vec3 camera;
uniform float viewDistance;

float fog_factor;
float fog_height;

const float pi = 3.14159265;
const float intensities[6]=float[6](0.7, 0.7, 1.0, 0.6, 0.85, 0.85);


void main()
{
	//fog
	vec4 sky_color=vec4(0.6, 0.8, 1.0, 1.0);
	float camera_distance = distance(camera, pos);
	fog_factor = pow(clamp(camera_distance / viewDistance, 0.0, 1.0), 4.0);
	float dy = pos.y - camera.y;
	float dx = distance(pos.xz, camera.xz);
	fog_height = (atan(dy, dx) + pi / 2) / pi;

	int f=int(frag_face+0.5);

	color = texture(sampler, texcoord);

    if(color.a == 0.0f)
        discard;

	float intensity=intensities[f];

	color.rgb*=frag_ao*frag_light*intensity;

	color = mix(color, sky_color, fog_factor);
}
