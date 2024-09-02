#include <cerrno>
#include <cstring>
#include <sstream>
#include <fstream>

#include <glad/glad.h>

#include "shaders.hpp"

namespace Shaders {

    GLuint create_shader(std::string shader_path, GLenum type) {

        std::ifstream shader_file(shader_path);
        if (shader_file.fail()) {
            std::ostringstream err_msg_stream;
            err_msg_stream <<
                "Error: Failed to load shader file at \"" << shader_path <<"\": " << strerror(errno) << "\n";
            throw std::runtime_error(err_msg_stream.str());
        }

        std::ostringstream shader_src_stream;
        shader_src_stream << shader_file.rdbuf();
        std::string shader_src = shader_src_stream.str();
        const char* shader_src_cstr = shader_src.c_str();

        unsigned shader = glCreateShader(type);
        glShaderSource(shader, 1, &shader_src_cstr, NULL);
        glCompileShader(shader);

        //Error checking
        int success;
        char info_log[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, NULL, info_log);
            std::ostringstream err_msg_stream;
            err_msg_stream <<
                "Failed to compile shader \"" << shader_path << "\"!\n" << info_log << "\n";
            throw std::runtime_error(err_msg_stream.str());
        }

        return shader;

    }

    GLuint link_shaders(GLuint *shaders, std::size_t n_shaders, std::string shader_program_name) {

        unsigned shader_program = glCreateProgram();

        for (std::size_t i = 0; i < n_shaders; i++) {
            glAttachShader(shader_program, shaders[i]);
        }

        glLinkProgram(shader_program);

        //Error checking
        int success;
        char info_log[512];
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            std::ostringstream err_msg_stream;
            err_msg_stream <<
                "Failed to link shader \"" << shader_program_name << "\"!\n" << info_log << "\n";
            throw std::runtime_error(err_msg_stream.str());
        }

        return shader_program;

    }

}
