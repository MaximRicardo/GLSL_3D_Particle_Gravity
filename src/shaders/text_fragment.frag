#version 430 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D screen_texture;

void main() {

    frag_color = texture(screen_texture, tex_coords);

}
