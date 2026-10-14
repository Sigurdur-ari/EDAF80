#version 410

layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 texcoords;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec2 uv;
	float world_z;
} vs_out;

void main()
{
	// pass uv coordinates and world z to fragment shader
	vs_out.uv = texcoords.xy;
	vec4 world_pos = vertex_model_to_world * vec4(vertex, 1.0);
	vs_out.world_z = world_pos.z;
	gl_Position = vertex_world_to_clip * world_pos;
}
