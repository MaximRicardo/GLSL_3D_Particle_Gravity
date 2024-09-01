#pragma once

#include <cstddef>
#include <string>

#include <glad/glad.h>

namespace Shaders {

    unsigned create_shader(std::string path, GLenum type);
    unsigned link_shaders(unsigned *shaders, std::size_t n_shaders, std::string shader_name); 

}
