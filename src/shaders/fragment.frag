#version 430 core

layout (location=0) out vec4 frag_color;
layout (location=1) out vec4 bright_color;

in vec4 color;

void main() {
    frag_color = color;

    //Is the fragment bright enough, if so output as a brightness color
    float brightness = dot(frag_color.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0)
        bright_color = frag_color;
    else
        bright_color = vec4(0.0, 0.0, 0.0, 1.0);
} 
