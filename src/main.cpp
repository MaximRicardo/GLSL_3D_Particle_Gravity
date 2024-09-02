#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
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
#include "sphere.hpp"
#include "camera.hpp"

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

glm::vec2 get_cursor_pos(GLFWwindow *window) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    return glm::vec2(x, y);
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
    GLuint shader_program;
    try {
        GLuint vertex_shader = Shaders::create_shader(exe_folder + "../src/shaders/vertex.vert", GL_VERTEX_SHADER);

        GLuint fragment_shader = Shaders::create_shader(exe_folder + "../src/shaders/fragment.frag", GL_FRAGMENT_SHADER);

        std::vector<GLuint> shaders = {vertex_shader, fragment_shader};
        shader_program = Shaders::link_shaders(shaders.data(), shaders.size(), "shader_program");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }
    catch (std::exception &e) {
        std::fprintf(stderr, "%s", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }

    std::vector<float> vertices = Sphere::sphere_vertices(16, 16, 0.5f);

    unsigned vao;
    glGenVertexArrays(1, &vao);

    unsigned vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //w component is ignored
    std::vector<glm::vec4> particle_positions(2);
    particle_positions[0] = glm::vec4(0.f, 0.f, -3.f, 0.f);
    particle_positions[1] = glm::vec4(3.f, 0.f, -3.f, 0.f);

    //w component is ignored
    std::vector<glm::vec4> particle_velocities(2, glm::vec4(0.f, 0.f, 0.f, 0.f));

    //w component is ignored
    std::vector<glm::vec4> particle_accelerations(2, glm::vec4(0.f, 0.f, 0.f, 0.f));
    particle_accelerations[0].x = 10.f;

    //SSBOs

    GLuint particle_positions_ssbo;
    glGenBuffers(1, &particle_positions_ssbo);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_positions_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_positions.size()*sizeof(particle_positions[0]), glm::value_ptr(particle_positions[0]), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint particle_velocities_ssbo;
    glGenBuffers(1, &particle_velocities_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_velocities_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_velocities.size()*sizeof(particle_velocities[0]), glm::value_ptr(particle_velocities[0]), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint particle_accelerations_ssbo;
    glGenBuffers(1, &particle_accelerations_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_accelerations_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_accelerations.size()*sizeof(particle_accelerations[0]), glm::value_ptr(particle_accelerations[0]), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    Camera::Camera camera;
    camera.MovementSpeed = 1.f;
    camera.MouseSensitivity = 0.05f;
    camera.Zoom = 1.f;

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;

    glm::vec2 start_cursor_pos = get_cursor_pos(window);
    glm::vec2 end_cursor_pos = start_cursor_pos;

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {

        float delta_time = std::chrono::duration<float>(end_time - start_time).count();
        float fps = (delta_time == 0.f) ? 0.f : 1.f/delta_time;

        glm::vec2 cursor_delta = end_cursor_pos - start_cursor_pos;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera::Camera_Movement::FORWARD, delta_time);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera::Camera_Movement::BACKWARD, delta_time);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera::Camera_Movement::LEFT, delta_time);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            camera.ProcessKeyboard(Camera::Camera_Movement::RIGHT, delta_time);
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
            camera.Position.y -= camera.MovementSpeed*delta_time;
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
            camera.Position.y += camera.MovementSpeed*delta_time;

        camera.ProcessMouseMovement(cursor_delta.x, -cursor_delta.y);

        glm::mat4 cam_projection_mat = glm::perspective(glm::radians(60.f), static_cast<float>(screen_width)/static_cast<float>(screen_height), 0.01f, 1000.f);
        glm::mat4 cam_mat = cam_projection_mat * camera.GetViewMatrix();//* cam_look_at_mat * cam_translate_mat;

        glClearColor(0.f, 0.0f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(shader_program);

        glUniformMatrix4fv(glGetUniformLocation(shader_program, "vp_mat"), 1, GL_FALSE, glm::value_ptr(cam_mat));

        GLint position_ssbo_binding = 0;
        GLint block_index = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BUFFER, "particle_positions_buffer");
        glShaderStorageBlockBinding(shader_program, block_index, particle_positions_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, position_ssbo_binding, particle_positions_ssbo);

        GLint acceleration_ssbo_binding = 1;
        block_index = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BUFFER, "particle_accelerations_buffer");
        glShaderStorageBlockBinding(shader_program, block_index, particle_accelerations_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, acceleration_ssbo_binding, particle_accelerations_ssbo);

        glBindVertexArray(vao);
        glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), 2);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, position_ssbo_binding, 0);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, acceleration_ssbo_binding, 0);
        glUseProgram(0);

        glfwSwapBuffers(window);
        glfwPollEvents();

        start_cursor_pos = end_cursor_pos;
        end_cursor_pos = get_cursor_pos(window);

        start_time = end_time;
        end_time = std::chrono::high_resolution_clock::now();
    }

    glfwTerminate();
    return 0;

}
