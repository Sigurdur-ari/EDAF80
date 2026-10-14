#version 410

uniform vec3 random_color;
uniform float t;

in VS_OUT {
	vec2 uv;
	float world_z;
} fs_in;

out vec4 frag_color;

void main()
{
	// calculate distance from face border
	vec2 edge_dist = min(fs_in.uv, vec2(1.0) - fs_in.uv);
	float dist = min(edge_dist.x, edge_dist.y);

	// smooth neon glow on borders
	float edge = 1.0 - smoothstep(0.03, 0.08, dist);

	// danger increases as cube gets closer to player
	float danger = clamp((fs_in.world_z + 28.0) / 28.0, 0.0, 1.0);

	// pulse speed increases as danger rises
	float pulse_speed = mix(3.0, 14.0, danger);
	float pulse = sin(t * pulse_speed) * 0.5 + 0.5;

	// red warning color
	vec3 warning_color = vec3(1.0, 0.08, 0.08);

	// blend base neon with warning pulse
	vec3 active_neon = mix(random_color, warning_color, danger * pulse * 0.85);

	// mix dark inner core with bright neon edges
	vec3 core_color = active_neon * 0.15;
	vec3 glow_color = active_neon * 2.2 + vec3(0.2);
	vec3 final_color = mix(core_color, glow_color, edge);

	// flash inner core when very close
	final_color += warning_color * (danger * pulse * 0.25);

	frag_color = vec4(final_color, 1.0);
}
