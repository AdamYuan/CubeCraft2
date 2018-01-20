#version 330 core
in vec3 frag_lighting;
in vec3 frag_pos;
in vec3 frag_texcoord;

out vec4 color;
uniform sampler2DArray sampler;
uniform sampler2D skySampler;
uniform vec3 camera;
uniform vec3 selection;
uniform float viewDistance;
uniform float dayTime;

float fog_factor;
float fog_height;

const float pi = 3.14159265;

void main()
{
	color = texture(sampler, frag_texcoord);
	if(color.a == 0.0f)
		discard;

	//fog
	float camera_distance = distance(camera, frag_pos);
	fog_factor = pow(clamp(camera_distance / viewDistance, 0.0, 1.0), 4.0);
	float dy = frag_pos.y - camera.y;
	float dx = distance(frag_pos.xz, camera.xz);
	fog_height = (atan(dy, dx) + pi / 2) / pi;

    vec3 sky_color = vec3(texture2D(skySampler, vec2(dayTime, 1.0f - fog_height)));

	vec3 color3 = color.rgb;
	color3 *= frag_lighting.x * frag_lighting.y * frag_lighting.z;
	//gamma correction
	float gamma = 0.8;
	color3 = pow(color3, vec3(1.0 / gamma));
	color3 = mix(color3, sky_color, fog_factor);

	//show selection box
	const float delta = 0.0001f;
	vec3 min_pos = selection - vec3(delta);
	vec3 max_pos = selection + vec3(1.0f + delta);
	if(
		(frag_pos.x >= min_pos.x && frag_pos.x <= max_pos.x) &&
		(frag_pos.y >= min_pos.y && frag_pos.y <= max_pos.y) &&
		(frag_pos.z >= min_pos.z && frag_pos.z <= max_pos.z))
		color3 *= 1.5f;

	color = vec4(color3, color.a);
}
