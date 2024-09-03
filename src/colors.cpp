#include <array>
#include <cstdint>

#include "random.hpp"
#include "colors.hpp"

namespace {

    glm::vec4 rgb8_to_f(std::uint8_t r, std::uint8_t g, std::uint8_t b) {

        glm::vec4 float_color;
        float_color.x = static_cast<float>(r)/255.f;
        float_color.y = static_cast<float>(g)/255.f;
        float_color.z = static_cast<float>(b)/255.f;

        return float_color;

    }

    bool is_in_range(float min, float max, float x) {
        return x >= min && x <= max;
    }

    std::array<float, Colors::n_star_colors> star_probabilities = {
        76.5f,  //M-type
        12.1f,  //K-type
        7.6f,   //G-Type
        3.f,    //F-type
        0.6f,   //A-type
        0.13f,  //B-type
    };

    /*
    std::array<float, Colors::n_star_colors> star_probabilities = {
        0.13f,  //B-type
        0.6f,   //A-type
        3.f,    //F-type
        7.6f,   //G-Type
        76.5f,  //M-type
        12.1f,  //K-type
    };*/

}

namespace Colors {

    std::array<glm::vec4, n_star_colors> star_colors = {
        rgb8_to_f(255, 100, 50),   //M-type
        rgb8_to_f(247, 167, 103),   //K-type
        rgb8_to_f(255, 226, 147),   //G-type
        rgb8_to_f(255, 248, 186),   //F-type
        rgb8_to_f(150, 182, 239),   //A-type
        rgb8_to_f(121, 125, 196),   //B-type
    };
    
    std::size_t rand_star_color_idx() {

        //return RandomNum::random_float(0.f, n_star_colors-1);

        float rand_val = RandomNum::random_float(0.f, 100.f);

        for (std::size_t i = 0; i < n_star_colors; i++) {
            if (is_in_range(0.f, star_probabilities[i], rand_val)) return i;
            rand_val -= static_cast<float>(star_probabilities[i]);
        }

        return star_probabilities[n_star_colors-1];

    }

}
