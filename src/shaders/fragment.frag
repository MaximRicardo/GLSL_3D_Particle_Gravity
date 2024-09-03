#version 430 core

out vec4 frag_color;

in vec4 color;

void main() {
    frag_color = vec4(
        clamp(color.x, 0.0, 1.0),
        clamp(color.y, 0.0, 1.0),
        clamp(color.z, 0.0, 1.0),
        clamp(color.w, 0.0, 1.0)
    );

} 
