#version 430 core
layout (location = 0) in vec3 v;
uniform mat4 mvp;

void main() {
    vec4 final_pos = mvp * vec4(v.xyz, 1.0);
    gl_Position = final_pos;
}
