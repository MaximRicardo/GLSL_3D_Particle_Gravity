#version 430 core
layout (location = 0) in vec3 v;

layout (std430, binding=0) buffer particle_positions_buffer {
    vec4 particle_positions[];
};

uniform mat4 vp_mat;

void main() {
    vec4 final_pos = vp_mat * vec4(particle_positions[gl_InstanceID].xyz + v.xyz, 1.0);
    gl_Position = final_pos;
}
