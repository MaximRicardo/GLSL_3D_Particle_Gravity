#include <cmath>

#include "random.hpp"
#include "galaxy.hpp"

namespace {

    constexpr float PI = 3.141592;

    constexpr float core_x_dist = 66.f;
    constexpr float core_z_dist = 66.f;
    
    constexpr float outer_core_x_dist = 100.f;
    constexpr float outer_core_z_dist = 100.f;

    constexpr float galaxy_thickness = 10.f;

    constexpr float arm_x_dist = 100.f;
    constexpr float arm_z_dist = 50.f;
    constexpr float arm_x_mean = 200.f;
    constexpr float arm_z_mean = 100.f;

    constexpr unsigned spiral = 3;
    constexpr unsigned arms = 2;

    glm::vec4 spiral_point(glm::vec3 p, float offset) {
        float r = std::sqrt(p.x*p.x+p.z*p.z);
        float theta = offset;
        theta += p.x > 0.f ? std::atan(p.z/p.x) : std::atan(p.z/p.x)*PI;
        theta += (r/arm_x_dist) * spiral;
        return glm::vec4(r*std::cos(theta), p.y, r*std::sin(theta), 1.f);
    }

}

namespace Galaxy {

    std::vector<glm::vec4> generate_galaxy(std::size_t n_points, glm::vec3 center) {

        std::vector<glm::vec4> points;

        glm::vec4 center_as_vec4 = glm::vec4(center, 0.f);

        for (std::size_t i = 0; i < n_points/4; i++) {
            points.push_back(glm::vec4(
                        RandomNum::gaus_rand(0, core_x_dist), RandomNum::gaus_rand(0, galaxy_thickness), RandomNum::gaus_rand(0, core_z_dist), 0.f
                    )+center_as_vec4);
        }

        for (std::size_t i = 0; i < n_points/4; i++) {
            points.push_back(glm::vec4(
                        RandomNum::gaus_rand(0, outer_core_x_dist), RandomNum::gaus_rand(0, galaxy_thickness), RandomNum::gaus_rand(0, outer_core_z_dist), 0.f
                    )+center_as_vec4);
        }

        for (std::size_t i = 0; i < arms; i++) {
            for (std::size_t j = 0; j < n_points/(arms*2); j++) {
                points.push_back(
                        spiral_point(glm::vec3(RandomNum::gaus_rand(arm_x_mean, arm_x_dist), RandomNum::gaus_rand(0.f, galaxy_thickness), RandomNum::gaus_rand(arm_z_mean, arm_z_dist)), (float)i*2.f*PI/(float)arms)+center_as_vec4
                        );
            }
        }

        return points;

    }

}
