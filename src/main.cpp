#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <array>

//The world if c/c++ was cross-platform: https://www.youtube.com/watch?v=RiZQoXCbcgo
#ifdef __unix__
    #include <unistd.h>
#endif
#ifdef __WIN32
    #include <windows.h>
#endif

//OpenGL stuff
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//Linear algebra stuff
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

std::string get_exe_path() {

    const std::size_t path_max = 1024;
    char buff[path_max];
    
    #ifdef __unix__
        ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
        if (len != -1) {
            buff[len] = '\0';
            return std::string(buff);
        }
        else {
            std::ostringstream err_msg_stream;
            err_msg_stream <<
                "Error: Couldn't get executable path: " << std::strerror(errno) << "\n";
            throw std::runtime_error(err_msg_stream.str());
        }
    #endif
    #ifdef __WIN32
        HMODULE hModule = GetModuleHandle(NULL);
        if (hModule != NULL) {
            GetModuleFileName(hModule, buff, sizeof(buff));
            return std::string(buff);
        }
        else {
            exit(1);  //Idk how tf to deal with this on windows
        }
    #endif

}

int main() {

    std::string exe_path = get_exe_path();
    std::string exe_folder = exe_path;
    while (exe_folder.back() != '/' && exe_folder.length() != 0) exe_folder.pop_back();

    //Initialize and configure glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    gladLoadGL();

    //Create a glfw window
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Hello Triangle", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    gladLoadGL();

    //Load in the shaders
    unsigned shader_program;
    {
        std::ifstream vertex_shader_file(exe_folder + "../src/shaders/vertex.vert");
        if (vertex_shader_file.fail()) {
            std::fprintf(stderr, "Error: Couldn't open vertex shader: %s\n", strerror(errno));
            glfwTerminate();
            return EXIT_FAILURE;
        }

        std::ostringstream vertex_shader_src_stream;
        vertex_shader_src_stream << vertex_shader_file.rdbuf();
        std::string vertex_shader_src = vertex_shader_src_stream.str();
        const char* vertex_shader_src_cstr = vertex_shader_src.c_str();

        unsigned vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, &vertex_shader_src_cstr, NULL);
        glCompileShader(vertex_shader);
        int success;
        char info_log[512];
        glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertex_shader, 512, NULL, info_log);
            std::fprintf(stderr, "Failed to compile vertex shader!\n%s\n", info_log);
            glfwTerminate();
            return EXIT_FAILURE;
        }

        std::ifstream fragment_shader_file(exe_folder + "../src/shaders/fragment.frag");
        if (fragment_shader_file.fail()) {
            std::fprintf(stderr, "Error: Couldn't open fragment shader: %s\n", strerror(errno));
            glfwTerminate();
            return EXIT_FAILURE;
        }
        std::ostringstream fragment_shader_src_stream;
        fragment_shader_src_stream << fragment_shader_file.rdbuf();
        std::string fragment_shader_src = fragment_shader_src_stream.str();
        const char* fragment_shader_src_cstr = fragment_shader_src.c_str();

        unsigned fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, &fragment_shader_src_cstr, NULL);
        glCompileShader(fragment_shader);
        glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragment_shader, 512, NULL, info_log);
            std::fprintf(stderr, "Failed to compile vertex shader!\n%s\n", info_log);
            glfwTerminate();
            return EXIT_FAILURE;
        }

        shader_program = glCreateProgram();
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_program, 512, NULL, info_log);
            std::fprintf(stderr, "Failed to link shaders!\n%s\n", info_log);
            glfwTerminate();
            return EXIT_FAILURE;
        }

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }

    std::array<float, 3*3> vertices = {
        -0.5f, -0.5f, -0.9f,
        0.5f, -0.5f, -0.9f,
        0.0f,  0.5f, -0.9f
    };

    unsigned vao;
    glGenVertexArrays(1, &vao);

    unsigned vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glm::vec3 cam_pos = {0.f, 0.f, 0.f};

    while (!glfwWindowShouldClose(window)) {

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam_pos.z += 0.001;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam_pos.z -= 0.001;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam_pos.x += 0.001;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam_pos.x -= 0.001;

        glm::mat4 projection_mat = glm::perspective(glm::radians(60.f), 1280.f/720.f, 0.01f, 100.f);

        glm::mat4 view_mat = glm::lookAt(
                cam_pos,
                glm::vec3(0.f, 0.f, -100.f),
                glm::vec3(0.f, 1.f, 0.f)
                );

        glm::mat4 translate_mat = glm::mat4(1.f);
        translate_mat = glm::translate(translate_mat, cam_pos);
        glm::mat4 rotate_mat = glm::mat4(1.f);
        rotate_mat = glm::rotate(rotate_mat, 0.f, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 scale_mat = glm::mat4(1.f);
        scale_mat = glm::scale(scale_mat, glm::vec3(1.f, 1.f, 1.f));

        //glm::mat4 model_mat = translate_mat*rotate_mat*scale_mat;

        glm::mat4 mvp = projection_mat * translate_mat;

        glClearColor(0.f, 0.0f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader_program);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;

}
