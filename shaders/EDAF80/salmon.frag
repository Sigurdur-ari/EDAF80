//
//  salmon.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 14.10.2026.
//

#version 410

uniform vec3 camera_position;
uniform vec3 light_position;

//Values/uniforms from node
uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;

// Textures
uniform sampler2D diffuse_texture;
uniform sampler2D specular_map;
uniform samplerCube skybox_texture;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;


void main()
{
	//BASIC PHONG SHADING
	
	// Initialize the normal
	vec3 n = normalize(fs_in.normal);

	//Find vector L pointing to the light source from the vertex
	vec3 L = normalize(light_position - fs_in.vertex);
	//Find vector V pointing to the camera from the vertex
	vec3 V = normalize(camera_position - fs_in.vertex);

	vec3 diffuse = diffuse_colour;
	
	vec3 specular = specular_colour;

	//DEMO CHECK for debugging DOES NOT HELP MUCH
	if (false){
		//Find the texture colors
		diffuse = texture(diffuse_texture, fs_in.normal.xy).rgb * diffuse_colour;
		
		// Get the specular strength from the specular map.
		specular = texture(specular_map, fs_in.normal.xy).rgb * specular_colour;
	}
	
	vec3 phong_colour = ambient_colour + diffuse * max(dot(n, L), 0.0) + specular * pow(max(dot(reflect(-L, n), V),0.0 ), shininess_value);
	
	// ADD REFLECTIONS AND FRESNEL CALCULATIONS
	
	// Reflections
	vec3 R = reflect(-V, n);
	vec3 reflection_colour = texture(skybox_texture, normalize(R)).rgb;
	
	// Fresnel
	//CHANGE THIS TO MAKE SHINY
	float R_0 = 0.4;
	float fresnel = R_0 + (1.0 - R_0) * pow((1.0 - dot(V, n)), 5.0);
	
	vec3 final_colour = mix(phong_colour, reflection_colour, fresnel);

		
	frag_color = vec4(final_colour, 1.0);

}
