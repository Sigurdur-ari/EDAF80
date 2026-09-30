#version 410
//
//  water.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 29.9.2026.
//

uniform vec3 camera_position;
uniform samplerCube skybox_texture;
uniform sampler2D wave_texture;

uniform float t;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 tangent;
	vec3 binormal;
	vec3 tex_coords;
} fs_in;

out vec4 water_color;

const vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
const vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);

void main()
{
	// Base color calculatiosn
	vec3 n = normalize(fs_in.normal);
	vec3 V = normalize(camera_position - fs_in.vertex);

	
	// Normal mapping
	vec2 texScale = vec2(8, 4);
	float normalTime = mod(t, 100.0);
	vec2 normalSpeed = vec2(-0.05, 0.0);

	vec2 normalCoord0 = fs_in.tex_coords.xz * texScale + normalTime * normalSpeed;
	vec2 normalCoord1 = fs_in.tex_coords.xz * texScale * 2 + normalTime * normalSpeed * 4;
	vec2 normalCoord2 = fs_in.tex_coords.xz * texScale * 4 + normalTime * normalSpeed * 8;
	
	
	vec4 n0 = texture(wave_texture, normalCoord0) * 2 - 1;
	vec4 n1 = texture(wave_texture, normalCoord1) * 2 - 1;
	vec4 n2 = texture(wave_texture, normalCoord2) * 2 - 1;
	
	vec3 n_bump = normalize(n0.xyz + n1.xyz + n2.xyz);
	
	// Create the TBN matrix used in normal mapping
	mat3 TBN = mat3(fs_in.tangent, fs_in.binormal, fs_in.normal);
	
	// Update n after normal mapping
	n = normalize(TBN * n_bump);
	
	// Reflections
	vec3 R = reflect(-V, n);
	vec4 reflection = texture(skybox_texture, normalize(R));
	
	// Fresnel
	float R_0 = 0.02037;
	float fresnel = R_0 + (1 - R_0) * pow((1 - dot(V, n)), 5);
	
	// Refraction
	float eta = 1.0/1.33;
	vec3 refract_vector = refract(-V, n, eta);
	vec4 refraction = texture(skybox_texture, normalize(refract_vector));
	
	// Calculate facing value after all initializations of n
	float facing = 1 - max(dot(V, n), 0);
	
	water_color = mix(color_deep, color_shallow, facing)
					+ reflection * fresnel
					+ refraction * (1 - fresnel);
	
}
