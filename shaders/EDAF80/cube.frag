//
//  cube.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 12.10.2026.
//

#version 410
uniform vec3 random_color;

out vec4 frag_color;

void main()
{
	frag_color = vec4(random_color, 1.0);
}
