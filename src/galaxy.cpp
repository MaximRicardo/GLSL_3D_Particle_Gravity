#include <cmath>

#include "galaxy.hpp"

namespace {

    constexpr float PI = 3.141592;

    constexpr float core_x_dist = 33.f;
    constexpr float core_z_dist = 33.f;
    
    constexpr float outer_core_x_dist = 100.f;
    constexpr float outer_core_z_dist = 100.f;

    constexpr float galaxy_thickness = 5.f;

    constexpr float arm_x_dist = 100.f;
    constexpr float arm_z_dist = 50.f;
    constexpr float arm_x_mean = 200.f;
    constexpr float arm_z_mean = 100.f;

    constexpr unsigned spiral = 3;
    constexpr unsigned arms = 2;

    float random_float(float a, float b) {
        float random = ((float) rand()) / (float) RAND_MAX;
        float diff = b - a;
        float r = random * diff;
        return a + r;
    }

    float gaus_rand(float mean, float stddev) {

        float u = 1 - random_float(0.f, 1.f);
        float v = random_float(0.f, 1.f);
        float z = std::sqrt(-2.f*std::log(u)) * std::cos(2.f*PI*v);

        return z*stddev+mean;

    }

    glm::vec4 spiral_point(glm::vec3 p, float offset) {
        float r = std::sqrt(p.x*p.x+p.z*p.z);
        float theta = offset;
        theta += p.x > 0 ? std::atan(p.z/p.x) : std::atan(p.z/p.x)*PI;
        theta += (r/arm_x_dist) * spiral;
        return glm::vec4(r*std::cos(theta), p.y, r*std::sin(theta), 1.f);
    }

}

namespace Galaxy {

    std::vector<glm::vec4> generate_galaxy(std::size_t n_points, float radius) {

        std::vector<glm::vec4> points;

        for (std::size_t i = 0; i < n_points/4; i++) {
            points.push_back(glm::vec4(
                        gaus_rand(0, core_x_dist), gaus_rand(0, galaxy_thickness), gaus_rand(0, core_z_dist), 0.f
                    ));
        }

        for (std::size_t i = 0; i < n_points/4; i++) {
            points.push_back(glm::vec4(
                        gaus_rand(0, outer_core_x_dist), gaus_rand(0, galaxy_thickness), gaus_rand(0, outer_core_z_dist), 0.f
                    ));
        }

        for (std::size_t i = 0; i < arms; i++) {
            for (std::size_t j = 0; j < n_points/4; j++) {
                points.push_back(
                        spiral_point(glm::vec3(gaus_rand(arm_x_mean, arm_x_dist), gaus_rand(0.f, galaxy_thickness), gaus_rand(arm_z_mean, arm_z_dist)), (float)i*2.f*PI/(float)arms)
                        );
            }
        }

        return points;

    }

}
