#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
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

#include "debug.hpp"
#include "shaders.hpp"
#include "sphere.hpp"
#include "camera.hpp"
#include "galaxy.hpp"

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
            exit(EXIT_FAILURE); //Idk how tf to deal with this on windows
        }
    #endif

}

glm::vec2 get_cursor_pos(GLFWwindow *window) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    return glm::vec2(x, y);
}

float random_float(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
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
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    //Create a glfw window
    constexpr unsigned screen_width = 1280;
    constexpr unsigned screen_height = 720;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Gravity Simulator", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    gladLoadGL();

    {
        int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            // initialize debug output
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(Debug::glDebugOutput, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        }
    }

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

    //Load in the physics compute shader
    GLuint physics_shader_program;
    unsigned physics_shader_local_group_size_x = 64;
    try {
        GLuint compute_shader = Shaders::create_shader(exe_folder + "../src/shaders/physics.comp", GL_COMPUTE_SHADER);

        std::vector<GLuint> shaders = {compute_shader};
        physics_shader_program = Shaders::link_shaders(shaders.data(), shaders.size(), "compute_shader_program");

        glDeleteShader(compute_shader);
    }
    catch (std::exception &e) {
        std::fprintf(stderr, "%s", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    std::vector<float> vertices = Sphere::sphere_vertices(16, 16, 1.f);

    unsigned vao;
    glGenVertexArrays(1, &vao);

    unsigned vbo;
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    std::size_t n_base_colors = 3;
    std::vector<glm::vec4> base_colors(n_base_colors);
    base_colors[0] = glm::vec4(1.f, 1.f, 1.f, 1.f);         //White
    base_colors[1] = glm::vec4(0.f, 0.47f, 0.95f, 1.f);     //Blue
    base_colors[2] = glm::vec4(0.9f, 0.16f, 0.22f, 1.f);    //Red

    for (std::size_t i = 0; i < n_base_colors; i++) base_colors[i] = glm::vec4(glm::normalize(glm::vec3(base_colors[i])), base_colors[i].w);

    std::size_t n_particles = 10000;

    //w component is ignored
    std::vector<glm::vec4> particle_positions = Galaxy::generate_galaxy(n_particles, 100.f);

    //w component is ignored
    std::vector<glm::vec4> particle_velocities(n_particles, glm::vec4(0.f, 0.f, 0.f, 0.f));

    //y, z and w components are ignored, std430 rounds everything up to vec4 sizes
    std::vector<glm::vec4> particle_lighting(n_particles);

    std::vector<glm::vec4> particle_base_colors(n_particles);

    for (std::size_t i = 0; i < n_particles; i++) {

        //Starting velocities

        float dist = glm::distance(glm::vec3(particle_positions[i]), glm::vec3(0.f, 0.f, 0.f));
        glm::vec3 up(0.f, 1.f, 0.f);
        glm::vec3 dir = glm::cross(up, glm::normalize(glm::vec3(particle_positions[i])));

        float mult = 0.1f;
        particle_velocities[i] = glm::vec4(dir*mult*dist, 1.f);

        //Base colors

        unsigned idx = std::round(random_float(0.f, n_base_colors-1));
        particle_base_colors[i] = base_colors[idx];

    }

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

    GLuint particle_lighting_ssbo;
    glGenBuffers(1, &particle_lighting_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_lighting_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_lighting.size()*sizeof(particle_lighting[0]), particle_lighting.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    GLuint particle_base_colors_ssbo;
    glGenBuffers(1, &particle_base_colors_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_base_colors_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_base_colors.size()*sizeof(particle_base_colors[0]), glm::value_ptr(particle_base_colors[0]), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    Camera::Camera camera(glm::vec3(0.f, 0.f, 100.f));
    camera.MovementSpeed = 25.f;
    camera.MouseSensitivity = 0.05f;
    camera.Zoom = 1.f;

    /*
    {
        GLint block_index = glGetProgramResourceIndex(physics_shader_program, GL_SHADER_STORAGE_BUFFER, "particle_positions_buffer");
        glShaderStorageBlockBinding(physics_shader_program, block_index, particle_positions_ssbo);
        
        block_index = glGetProgramResourceIndex(physics_shader_program, GL_SHADER_STORAGE_BUFFER, "particle_accelerations_buffer");
        glShaderStorageBlockBinding(physics_shader_program, block_index, particle_accelerations_ssbo);
        
        block_index = glGetProgramResourceIndex(physics_shader_program, GL_SHADER_STORAGE_BUFFER, "particle_velocities_buffer");
        glShaderStorageBlockBinding(physics_shader_program, block_index, particle_velocities_ssbo);


        block_index = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BUFFER, "particle_positions_buffer");
        glShaderStorageBlockBinding(shader_program, block_index, particle_positions_ssbo);
        
        block_index = glGetProgramResourceIndex(shader_program, GL_SHADER_STORAGE_BUFFER, "particle_accelerations_buffer");
        glShaderStorageBlockBinding(shader_program, block_index, particle_accelerations_ssbo);
    }
    */

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;

    glm::vec2 start_cursor_pos = get_cursor_pos(window);
    glm::vec2 end_cursor_pos = start_cursor_pos;

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {

        float delta_time = std::chrono::duration<float>(end_time - start_time).count();
        float fps = (delta_time == 0.f) ? 0.f : 1.f/delta_time;

        glm::vec2 cursor_delta = end_cursor_pos - start_cursor_pos;

        //std::printf("fps = %f\n", fps);

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

        glm::mat4 cam_projection_mat = glm::perspective(glm::radians(60.f), static_cast<float>(screen_width)/static_cast<float>(screen_height), 0.01f, 10000.f);
        glm::mat4 cam_mat = cam_projection_mat * camera.GetViewMatrix();//* cam_look_at_mat * cam_translate_mat;

        glClearColor(0.f, 0.0f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_positions_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particle_lighting_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particle_velocities_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_base_colors_ssbo);

        //physics

        glUseProgram(physics_shader_program);

        {
            glUniform1f(glGetUniformLocation(physics_shader_program, "G"), 1.f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "particle_mass"), 10.f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "particle_radius"), 1.f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "particle_light_strength"), 1.f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "delta_time"), delta_time);
            glUniform1i(glGetUniformLocation(physics_shader_program, "n_particles"), n_particles);

            glDispatchCompute(static_cast<GLuint>(std::ceil(static_cast<float>(n_particles)/physics_shader_local_group_size_x)), 1, 1);

            glMemoryBarrier(GL_ALL_BARRIER_BITS);

            glUseProgram(0);
        }

        //Rendering everything

        glUseProgram(shader_program);

        {
            glUniformMatrix4fv(glGetUniformLocation(shader_program, "vp_mat"), 1, GL_FALSE, glm::value_ptr(cam_mat)); //View and projection matrix
            
            glBindVertexArray(vao);
            glDrawArraysInstanced(GL_TRIANGLES, 0, vertices.size(), n_particles);

            glMemoryBarrier(GL_ALL_BARRIER_BITS);

            glUseProgram(0);
        }
        
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
