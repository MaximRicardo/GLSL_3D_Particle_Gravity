#version 430 core

layout (location = 0) in vec3 v;

layout (std430, binding=0) readonly buffer particle_positions_buffer {
    vec4 particle_positions[];
};

layout (std430, binding=1) readonly buffer particle_lighting_buffer {
    float particle_lighting[];
};

layout (std430, binding=3) readonly buffer particle_base_colors_buffer {
    vec4 particle_base_colors[];
};

layout (std430, binding=4) readonly buffer particle_radii_buffer {
    float particle_radii[];
};

uniform mat4 vp_mat;
uniform vec3 cam_pos;

out vec4 color;

void main() {
    float cam_dist = distance(particle_positions[gl_InstanceID].xyz, cam_pos.xyz);
    vec3 true_v = v * clamp(particle_radii[gl_InstanceID] * cam_dist/250.0, particle_radii[gl_InstanceID]/10.0, particle_radii[gl_InstanceID]);

    vec4 final_pos = vp_mat * vec4(particle_positions[gl_InstanceID].xyz + true_v.xyz, 1.0);
    gl_Position = final_pos;

    vec4 base_color = particle_base_colors[gl_InstanceID];
    color = vec4(base_color.xyz*particle_lighting[gl_InstanceID], cam_dist);  //The alpha component will contain the distance from the camera to the particle
}
