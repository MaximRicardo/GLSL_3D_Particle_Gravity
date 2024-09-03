#version 430 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D screen_texture;

void main() {

    frag_color = vec4(texture(screen_texture, tex_coords).rgb, 1.0);
    //frag_color = vec4(tex_coords, 0.0, 1.0);

}
