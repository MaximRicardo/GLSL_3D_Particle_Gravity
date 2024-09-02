#version 430 core

layout (location = 0) in vec3 v;

layout (std430, binding=0) buffer particle_positions_buffer {
    vec4 particle_positions[];
};

layout (std430, binding=1) buffer particle_accelerations_buffer {
    vec4 particle_accelerations[];
};

uniform mat4 vp_mat;

out vec4 color;

void main() {
    vec4 final_pos = vp_mat * vec4(particle_positions[gl_InstanceID].xyz + v.xyz, 1.0);
    gl_Position = final_pos;

    vec4 blue = vec4(0.0, 0.47, 0.95, 1.0);
    vec4 red = vec4(0.90, 0.16, 0.22, 1.0);

    float t = length(particle_accelerations[gl_InstanceID]) / 5.0;
    t = clamp(t, 0.0, 1.0);
    color = mix(blue, red, t);

    //particle_positions[gl_InstanceID].x += 0.01;
}
