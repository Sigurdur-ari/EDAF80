//
//  end_screen.frag
//  CG_Labs
//
//  Created by Sigurður Stefánsson on 13.10.2026.
//

#version 410

uniform sampler2D screen_texture;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main()
{
	frag_color = texture(screen_texture, fs_in.texcoord);
}
