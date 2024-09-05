#version 430 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D bloom_blur;

void main() {

    //frag_color = vec4(0.f, 0.f, 0.f, 0.f);
    //return;

    vec4 bloom_color = texture(bloom_blur, tex_coords).rgba;

    vec3 blue = vec3(0.4, 0.6, 1.0);
    vec3 bloom_as_blue = blue*sqrt(length(bloom_color.rgb));

    frag_color = vec4(bloom_as_blue, clamp(bloom_color.a/250.0, 0.0, 1.0));

}
