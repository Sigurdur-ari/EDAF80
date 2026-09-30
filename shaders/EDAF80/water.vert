#version 410
//
//  water.vert
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 29.9.2026.
//
layout (location = 0) in vec3 vertex;
layout (location = 2) in vec3 tex_coords;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

/* Add time uniform */
uniform float t;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 tex_coords;
} vs_out;

// Calculate wave
float wave(vec2 position, vec2 direction, float amplitude, float frequency,
		   float phase, float sharpness, float time)
{
	return amplitude * pow(sin((position.x * direction.x + position.y * direction.y)
							* frequency + phase * time) * 0.5 + 0.5, sharpness);
}

// Calculate derivative with respect to x
float derivative_x(vec2 position, vec2 direction, float amplitude, float frequency,
				   float phase, float sharpness, float time){
	float outside_part = 0.5 * sharpness * frequency * amplitude;
	
	float sin_part = pow(sin((position.x * direction.x + position.y * direction.y)
							 * frequency + phase * time) * 0.5 + 0.5, sharpness - 1);
	
	float cos_part = cos((position.x * direction.x + position.y * direction.y)
						 * frequency + phase * time) * direction.x;
	
	return outside_part * sin_part * cos_part;
}

// Calculate derivative with respect to y
float derivative_z(vec2 position, vec2 direction, float amplitude, float frequency,
				   float phase, float sharpness, float time){
	float outside_part = 0.5 * sharpness * frequency * amplitude;
	
	float sin_part = pow(sin((position.x * direction.x + position.y * direction.y)
							 * frequency + phase * time) * 0.5 + 0.5, sharpness - 1);
	
	float cos_part = cos((position.x * direction.x + position.y * direction.y)
						 * frequency + phase * time) * direction.y;
	
	return outside_part * sin_part * cos_part;
}


void main(){
	vec3 displaced_vertex = vertex;
	//Wave 1
	displaced_vertex.y += wave(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
	//Wave 2
	displaced_vertex.y += wave(vertex.xz, vec2(-0.7, -0.7), 0.5, 0.4, 1.3, 2.0, t);
	
	float der_x = 0.0;
	der_x += derivative_x(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
	der_x += derivative_x(vertex.xz, vec2(-0.7, -0.7), 0.5, 0.4, 1.3, 2.0, t);
	
	float der_z = 0.0;
	der_z += derivative_z(vertex.xz, vec2(-1.0, 0.0), 1.0, 0.2, 0.5, 2.0, t);
	der_z += derivative_z(vertex.xz, vec2(-0.7, -0.7), 0.5, 0.4, 1.3, 2.0, t);
	
	// Generate normal, tangent and bitangent
	vec3 wave_normal = vec3(-der_x, 1, -der_z);
	vec3 wave_tangent = vec3(1, der_x, 0);
	vec3 wave_binormal = vec3(0, der_z, 1);
	
	
	vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced_vertex, 1.0));
	vs_out.normal = vec3(normal_model_to_world * vec4(wave_normal, 0.0));
	vs_out.tangent = vec3(normal_model_to_world * vec4(wave_tangent, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(wave_binormal, 0.0));
	vs_out.tex_coords = tex_coords;
	
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced_vertex, 1.0);
}
