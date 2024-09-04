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

    /*
    std::array<float, Colors::n_star_colors> star_probabilities = {
        76.5f,  //M-type
        12.1f,  //K-type
        7.6f,   //G-Type
        3.f,    //F-type
        0.6f,   //A-type
        0.13f,  //B-type
    };*/

}

namespace Colors {

    std::array<glm::vec4, n_star_colors> star_colors = {
        rgb8_to_f(0xff, 0x53, 0x49),  //M-type
        rgb8_to_f(0xff, 0xae, 0x42),  //K-type
        rgb8_to_f(0xf4, 0xa0, 0x3f),  //G-type
        rgb8_to_f(0xfc, 0xf3, 0xcf),  //F-type
        rgb8_to_f(0xff, 0xff, 0xff),  //A-type
        rgb8_to_f(0xd6, 0xea, 0xf8),  //B-type
    };
    
    std::size_t rand_star_color_idx() {

        return std::round(RandomNum::random_float(0.f, n_star_colors-1));

        /*
        float rand_val = RandomNum::random_float(0.f, 100.f);

        for (std::size_t i = 0; i < n_star_colors; i++) {
            if (is_in_range(0.f, star_probabilities[i], rand_val)) return i;
            rand_val -= static_cast<float>(star_probabilities[i]);
        }

        return star_probabilities[n_star_colors-1];*/

    }

}
