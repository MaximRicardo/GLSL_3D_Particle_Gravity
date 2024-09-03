#version 430 core

layout (location = 0) in vec2 v;
layout (location = 1) in vec2 a_tex_coords;

out vec2 tex_coords;

void main() {

    gl_Position = vec4(v, 0.0, 1.0);
    tex_coords = a_tex_coords;

}
