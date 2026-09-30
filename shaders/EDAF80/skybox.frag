#version 410
//
//  skybox.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 23.9.2026.

// Get the skybox cubemap texture
uniform samplerCube skybox_texture;

in VS_OUT {
	vec3 direction;
} fs_in;

out vec4 color;

void main(){
	
	// Calculate the pixel color by looking it up in the skybox texture. 
	color = texture(skybox_texture, normalize(fs_in.direction));
}
