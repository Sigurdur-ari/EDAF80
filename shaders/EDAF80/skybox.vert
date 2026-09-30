#version 410
//
//  skybox.vert
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 23.9.2026.
//

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

// Get the uniform camera position in world coordinates.
uniform vec3 camera_position;


out VS_OUT {
	vec3 direction;
} vs_out;

void main(){
	// Find the direction in which the vertex sits from the camera.
	// Camera is used to create the illusion of infinite background.
	//vs_out.direction = vec3(vertex_model_to_world * (vec4(vertex, 1.0)));
	
	vs_out.direction = vertex;
	
	// Calculate the position of the vertex in clip space
	gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
}
