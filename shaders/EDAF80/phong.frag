#version 410
//
//  phong.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 23.9.2026.
//

// Uniforms from set_phong_uniforms
uniform vec3 light_position;
uniform vec3 camera_position;
uniform bool use_normal_mapping;

//Values/uniforms from node
uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

// Uploaded textures
uniform sampler2D diffuse_texture;
uniform sampler2D specular_map;
uniform sampler2D normal_map;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec2 tex_coords;
	vec3 tangent;
	vec3 binormal;
} fs_in;

out vec4 color;

void main(){
	
	// Create the TBN matrix used in normal mapping
	mat3 TBN = mat3(fs_in.tangent, fs_in.binormal, fs_in.normal);
	
	// Initialize the normal
	vec3 n;
	if (use_normal_mapping) {
		
		//Calculate normal from normal map if normal mapping is enabled
		// multiplying it by 2 and subtracting one to get it in the range of normals [-1, 1] and then normalizing it because we only care about the direction not magnitude.
		vec3 normal_map_lookup = normalize(texture(normal_map, fs_in.tex_coords).rgb * 2.0 - 1.0);
	
		// Transform that normal into world space using TBN
		n = normalize(TBN * normal_map_lookup);
		
	}
	else{
		// Set world normal if normal mapping is disabled
		n = normalize(fs_in.normal);
	}
	//Find vector L pointing to the light source from the vertex
	vec3 L = normalize(light_position - fs_in.vertex);
	//Find vector V pointing to the camera from the vertex
	vec3 V = normalize(camera_position - fs_in.vertex);
	
	//Find the texture colors
	vec3 diffuse = texture(diffuse_texture, fs_in.tex_coords).rgb;
	
	// Get the specular strength from the specular map.
	vec3 specular_value = texture(specular_map, fs_in.tex_coords).rgb;
	
	// Another version where the value controls the strength and specular below controls color.
	//float specular_value = texture(specular_map, fs_in.tex_coords).r;
	// Calculate the total specular value by multiplying the specular vector with the calculated map value.
	//vec3 specular = specular_colour * specular_value;
	
	//NOW SPECULAR_COLOUR AND DIFFUSE_COLOR MATERIAL CONSTANTS HAVE NO EFFECT ON THE SPHERE
	
	// Calculate the final pixel color using the phong shader formula
	// Can be made more readable...
	color = vec4(ambient_colour + diffuse * max(dot(n, L), 0.0) + specular_value * pow(max(dot(reflect(-L, n), V),0.0 ), shininess_value), 1.0);

}
