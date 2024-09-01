#version 430 core
layout (location = 0) in vec3 v;

layout (std430, binding=1) buffer mvps {
    mat4 mvp_mats[];
};

void main() {
    vec4 final_pos = mvp_mats[gl_InstanceID] * vec4(v.xyz, 1.0);
    gl_Position = final_pos;
}
