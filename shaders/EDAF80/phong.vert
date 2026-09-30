#version 410
//
//  phong.vert
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 23.9.2026.
//

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 tex_coords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 tex_coords;
	vec3 tangent;
	vec3 binormal;
} vs_out;

void main(){
	// Transform points and directions to world space before sending them to the fragment shader.
	// Vertex gets 1.0 because its a point and should be affected by translations
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	//Normal as well as tangents/binormals get 0 because they are directions and should not be affected by translations.)
	vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
	vs_out.tex_coords = tex_coords.xy;
	vs_out.tangent = vec3(normal_model_to_world * vec4(tangent, 0.0));
	vs_out.binormal = vec3(normal_model_to_world * vec4(binormal, 0.0));
	 

	// Calculate the clip space position of this vertex
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}



