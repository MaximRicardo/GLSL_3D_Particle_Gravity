#version 430 core

out vec4 frag_color;

in vec2 tex_coords;

uniform sampler2D scene;
uniform sampler2D bloom_blur;
uniform bool bloom;
uniform float exposure;

void main() {

    //frag_color = vec4(texture(screen_texture, tex_coords).rgb, 1.0);

    const float gamma = 2.2;
    vec3 hdr_color = texture(scene, tex_coords).rgb;      
    vec3 bloom_color = texture(bloom_blur, tex_coords).rgb;
    if(bloom)
        hdr_color += bloom_color; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdr_color * exposure);
    // also gamma correct while we're at it       
    result = pow(result, vec3(1.0 / gamma));

    frag_color = vec4(result, 1.0);

}
