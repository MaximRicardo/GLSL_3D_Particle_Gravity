#pragma once

#include <cstddef>
#include <string>

#include <glad/glad.h>

namespace Shaders {

    GLuint create_shader(std::string path, GLenum type);
    GLuint link_shaders(GLuint *shaders, std::size_t n_shaders, std::string shader_name); 

}
