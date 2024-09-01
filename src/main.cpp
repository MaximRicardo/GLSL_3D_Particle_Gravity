#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <glm/ext/matrix_transform.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <chrono>

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
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

#include "shaders.hpp"

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
    constexpr unsigned screen_width = 1280;
    constexpr unsigned screen_height = 720;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Hello Triangle", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    gladLoadGL();

    //Load in the shaders
    unsigned shader_program;
    try {
        unsigned vertex_shader = Shaders::create_shader(exe_folder + "../src/shaders/vertex.vert", GL_VERTEX_SHADER);

        unsigned fragment_shader = Shaders::create_shader(exe_folder + "../src/shaders/fragment.frag", GL_FRAGMENT_SHADER);

        std::vector<unsigned> shaders = {vertex_shader, fragment_shader};
        shader_program = Shaders::link_shaders(shaders.data(), shaders.size(), "shader_program");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }
    catch (std::exception &e) {
        std::fprintf(stderr, "%s", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }

    std::array<float, 3*3> vertices = {
        -0.5f, -0.5f, 0.f,
        0.5f, -0.5f, 0.f,
        0.0f,  0.5f, 0.f
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
    float cam_move_speed = 1.f;

    //std::chrono::microseconds start_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;

    while (!glfwWindowShouldClose(window)) {

        float delta_time = std::chrono::duration<float>(end_time - start_time).count();
        float fps = (delta_time == 0.f) ? 0.f : 1.f/delta_time;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam_pos.z += cam_move_speed*delta_time;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam_pos.z -= cam_move_speed*delta_time;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam_pos.x += cam_move_speed*delta_time;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam_pos.x -= cam_move_speed*delta_time;

        glm::mat4 projection_mat = glm::perspective(glm::radians(60.f), static_cast<float>(screen_width)/static_cast<float>(screen_height), 0.01f, 1000.f);
        glm::mat4 cam_translate_mat = glm::mat4(1.f);
        cam_translate_mat = glm::translate(cam_translate_mat, cam_pos);

        glm::mat4 view_mat = glm::lookAt(
                cam_pos,
                glm::vec3(0.f, 0.f, -1000.f),
                glm::vec3(0.f, 1.f, 0.f)
                );

        glm::mat4 translate_mat = glm::mat4(1.f);
        translate_mat = glm::translate(translate_mat, glm::vec3(0.f, 0.f, -1.f));
        glm::mat4 rotate_mat = glm::mat4(1.f);
        rotate_mat = glm::rotate(rotate_mat, 0.785f, glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 scale_mat = glm::mat4(1.f);
        scale_mat = glm::scale(scale_mat, glm::vec3(2.f, 1.f, 1.f));

        glm::mat4 model_mat = translate_mat*rotate_mat*scale_mat;

        glm::mat4 mvp = projection_mat * cam_translate_mat * model_mat;

        glClearColor(0.f, 0.0f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(shader_program);
        glUniformMatrix4fv(glGetUniformLocation(shader_program, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glfwSwapBuffers(window);
        glfwPollEvents();

        start_time = end_time;
        end_time = std::chrono::high_resolution_clock::now();
    }

    std::puts("a");

    glfwTerminate();
    return 0;

}
