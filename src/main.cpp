#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <glm/ext/matrix_projection.hpp>
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
#include "colors.hpp"

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

    //Load in the final rendering shader
    GLuint final_shader_program;
    try {
        GLuint vertex_shader = Shaders::create_shader(exe_folder + "../src/shaders/final.vert", GL_VERTEX_SHADER);
        GLuint fragment_shader = Shaders::create_shader(exe_folder + "../src/shaders/final.frag", GL_FRAGMENT_SHADER);

        std::vector<GLuint> shaders = {vertex_shader, fragment_shader};
        final_shader_program = Shaders::link_shaders(shaders.data(), shaders.size(), "final_shader_program");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }
    catch (std::exception &e) {
        std::fprintf(stderr, "%s", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }

    //Load in the blurring shader
    GLuint blur_shader_program;
    try {
        GLuint vertex_shader = Shaders::create_shader(exe_folder + "../src/shaders/blur.vert", GL_VERTEX_SHADER);
        GLuint fragment_shader = Shaders::create_shader(exe_folder + "../src/shaders/blur.frag", GL_FRAGMENT_SHADER);

        std::vector<GLuint> shaders = {vertex_shader, fragment_shader};
        blur_shader_program = Shaders::link_shaders(shaders.data(), shaders.size(), "blur_shader_program");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }
    catch (std::exception &e) {
        std::fprintf(stderr, "%s", e.what());
        glfwTerminate();
        return EXIT_FAILURE;
    }
    
    std::vector<float> sphere_vertices = Sphere::sphere_vertices(10, 10, 1.f);

    GLuint vao;
    glGenVertexArrays(1, &vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);

    {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sphere_vertices.size()*sizeof(float), sphere_vertices.data(), GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    GLuint screen_vao;
    glGenVertexArrays(1, &screen_vao);

    GLuint screen_vbo;
    glGenBuffers(1, &screen_vbo);

    {
        std::array<float, 24> vertices = {  // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
             1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
             1.0f, -1.0f,  1.0f, 0.0f,
             1.0f,  1.0f,  1.0f, 1.0f
        };

        glBindVertexArray(screen_vao);
        glBindBuffer(GL_ARRAY_BUFFER, screen_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    }

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));

    std::size_t n_particles = 100000;
    float particle_mass = 10.f/(static_cast<float>(n_particles)/2000.f);

    std::array<glm::vec3, 1> galaxy_centers = {
        glm::vec3(-500.f, 0.f, 0.f),
        //glm::vec3(500.f, 0.f, 0.f)
    };

    //w component is ignored
    std::vector<glm::vec4> particle_positions = Galaxy::generate_galaxy(n_particles/galaxy_centers.size(), galaxy_centers[0]);
    for (std::size_t i = 0; i < galaxy_centers.size()-1; i++){
        std::vector<glm::vec4> other_particle_positions = Galaxy::generate_galaxy(n_particles/galaxy_centers.size(), galaxy_centers[1]);
        for (auto& particle_pos : other_particle_positions) particle_positions.push_back(particle_pos);
    }

    std::printf("particle_positions size = %zu\n", particle_positions.size());

    //w component is ignored
    std::vector<glm::vec4> particle_velocities(n_particles, glm::vec4(0.f, 0.f, 0.f, 0.f));

    //y, z and w components are ignored, std430 rounds everything up to vec4 sizes
    std::vector<glm::vec4> particle_lighting(n_particles);

    //same as with particle_lighting
    std::vector<glm::vec4> particle_radii(n_particles);

    std::vector<glm::vec4> particle_base_colors(n_particles);

    for (std::size_t i = 0; i < n_particles; i++) {

        //Starting velocities

        glm::vec3 galaxy_center = galaxy_centers[i/(n_particles/galaxy_centers.size())];

        float dist = glm::distance(glm::vec3(particle_positions[i]), galaxy_center);
        glm::vec3 up(0.f, 1.f, 0.f);
        glm::vec3 dir = glm::cross(glm::normalize(galaxy_center - glm::vec3(particle_positions[i])), up);

        //float mult = 25.f;
        float mult = 7.5f;
        if (dist < 200.f) mult = 0.05f*dist;
        //Half the particles are within the core
        if (dist < 66.f) mult = sqrt((particle_mass*n_particles/2.f)*dist)/125.f;
        if (dist < 33.f) mult = sqrt((particle_mass*n_particles/8.f))/100.f*0.f;
        particle_velocities[i] = glm::vec4(dir*mult, 1.f);

        //Base colors

        std::size_t idx = Colors::rand_star_color_idx();
        particle_base_colors[i] = glm::vec4(glm::normalize(glm::vec3(Colors::star_colors[idx])), 1.f);

        //Radii

        //sqrt(sqrt(x)) lez gooo
        particle_radii[i] = glm::vec4(std::sqrt(static_cast<float>(idx+1)), 0.f, 0.f, 0.f);

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

    GLuint particle_radii_ssbo;
    glGenBuffers(1, &particle_radii_ssbo);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particle_radii_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particle_radii.size()*sizeof(particle_radii[0]), particle_radii.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    Camera::Camera camera(glm::vec3(0.f, 0.f, 100.f));
    camera.MovementSpeed = 100.f;
    camera.MouseSensitivity = 0.05f;
    camera.Zoom = 1.f;

    GLuint hdr_fbo;
    glGenFramebuffers(1, &hdr_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo);

    GLuint hdr_rbo;
    glGenRenderbuffers(1, &hdr_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, hdr_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screen_width, screen_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, hdr_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    std::array<GLuint, 2> color_buffers;
    glGenTextures(2, color_buffers.data());
    for (unsigned i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, color_buffers[i]);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL
                );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //Attach the texture to the frame buffer
        glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0+i, GL_TEXTURE_2D, color_buffers[i], 0
                );
    }

    {
        std::array<GLuint, 2> attachments = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, attachments.data());
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::fprintf(stderr, "Error: hdr_fbo not ready!\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::array<GLuint, 2> ping_pong_fbo;
    std::array<GLuint, 2> ping_pong_color_buffers;
    glGenFramebuffers(2, ping_pong_fbo.data());
    glGenTextures(2, ping_pong_color_buffers.data());
    for (unsigned i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, ping_pong_fbo[i]);
        glBindTexture(GL_TEXTURE_2D, ping_pong_color_buffers[i]);
        glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA16F, screen_width, screen_height, 0, GL_RGBA, GL_FLOAT, NULL
                );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(
                GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ping_pong_color_buffers[i], 0
                );
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::fprintf(stderr, "Error: hdr_fbo not ready!\n");
    }

    //Setup the shader uniforms
    glUseProgram(blur_shader_program);
    glUniform1i(glGetUniformLocation(blur_shader_program, "image"), 0);
    glUseProgram(final_shader_program);
    glUniform1i(glGetUniformLocation(final_shader_program, "scene"), 0);
    glUniform1i(glGetUniformLocation(final_shader_program, "bloom_blur"), 1);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    bool use_bloom = true;
    bool r_still_down = false;

    bool paused = false;
    bool space_still_down = false;

    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time;

    glm::vec2 start_cursor_pos = get_cursor_pos(window);
    glm::vec2 end_cursor_pos = start_cursor_pos;

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS) {

        float delta_time = std::chrono::duration<float>(end_time - start_time).count();
        float fps = (delta_time == 0.f) ? 0.f : 1.f/delta_time;

        glm::vec2 cursor_delta = end_cursor_pos - start_cursor_pos;

        std::printf("fps = %f\n", fps);

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

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && !r_still_down) {
            use_bloom = !use_bloom;
            r_still_down = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE) r_still_down = false;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !space_still_down) {
            paused = !paused;
            space_still_down = true;
        }
        else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)  space_still_down = false;

        camera.ProcessMouseMovement(cursor_delta.x, -cursor_delta.y);

        glm::mat4 cam_projection_mat = glm::perspective(glm::radians(60.f), static_cast<float>(screen_width)/static_cast<float>(screen_height), 0.01f, 10000.f);
        glm::mat4 cam_mat = cam_projection_mat * camera.GetViewMatrix();

        glBindFramebuffer(GL_FRAMEBUFFER, hdr_fbo);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particle_positions_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particle_lighting_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, particle_velocities_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, particle_base_colors_ssbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particle_radii_ssbo);

        //physics

        glUseProgram(physics_shader_program);

        if (!paused) {
            glUniform1f(glGetUniformLocation(physics_shader_program, "G"), 1.f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "particle_mass"), particle_mass);
            glUniform1f(glGetUniformLocation(physics_shader_program, "particle_light_strength"), 0.5f);
            glUniform1f(glGetUniformLocation(physics_shader_program, "delta_time"), delta_time);
            glUniform1i(glGetUniformLocation(physics_shader_program, "n_particles"), n_particles);

            glDispatchCompute(static_cast<GLuint>(std::ceil(static_cast<float>(n_particles)/physics_shader_local_group_size_x)), 1, 1);

            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            glUseProgram(0);
        }

        //Rendering everything

        glUseProgram(shader_program);

        {
            glUniformMatrix4fv(glGetUniformLocation(shader_program, "vp_mat"), 1, GL_FALSE, glm::value_ptr(cam_mat)); //View and projection matrix
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);

            glBindVertexArray(vao);
            glDrawArraysInstanced(GL_TRIANGLES, 0, sphere_vertices.size(), n_particles);

            glUseProgram(0);
        }

        //Blur bright fragments via a gaussian blur
        bool blur_horizontal = true;
        {
            bool first_iter = true;
            unsigned amount = 10;
            
            glUseProgram(blur_shader_program);

            glBindVertexArray(screen_vao);
            for (unsigned i = 0; i < amount; i++) {
                glBindFramebuffer(GL_FRAMEBUFFER, ping_pong_fbo[blur_horizontal]);
                glUniform1i(glGetUniformLocation(blur_shader_program, "horizontal"), blur_horizontal);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, first_iter ? color_buffers[1] : ping_pong_color_buffers[!blur_horizontal]);
                glDrawArrays(GL_TRIANGLES, 0, 6);
                blur_horizontal = !blur_horizontal;
                first_iter = false;
            }

            glUseProgram(0);
        }

        //Render the screen textures
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(final_shader_program);
        {
            glBindVertexArray(screen_vao);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, color_buffers[0]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, ping_pong_color_buffers[!blur_horizontal]);

            glUniform1i(glGetUniformLocation(final_shader_program, "bloom"), use_bloom);
            glUniform1f(glGetUniformLocation(final_shader_program, "exposure"), 0.1f);

            glDrawArrays(GL_TRIANGLES, 0, 6);
            
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
