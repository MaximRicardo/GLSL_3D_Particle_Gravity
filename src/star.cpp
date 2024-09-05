#include <array>
#include <cstdint>

#include "random.hpp"
#include "star.hpp"

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

}

namespace Star {

    std::array<glm::vec4, n_star_colors> star_colors = {
        rgb8_to_f(0xff, 0xcc, 0x6f),
        rgb8_to_f(0xff, 0xd2, 0xa1),
        rgb8_to_f(0xff, 0xf4, 0xea),
        rgb8_to_f(0xf8, 0xf7, 0xff),
        rgb8_to_f(0xca, 0xd7, 0xff),
        rgb8_to_f(0xaa, 0xbf, 0xff),
    };

    std::array<float, n_star_colors> star_size_mults = {
        2.f,
        2.f,
        2.f,
        2.0f,
        2.5f,
        3.5f,
    };
    
    std::array<float, n_star_colors> star_probabilities = {
        76.45f,
        12.1f,
        7.6f,
        3.f,
        0.6f,
        0.13f,
    };
    
    std::size_t rand_star_type_idx() {

        //return std::round(RandomNum::random_float(0.f, n_star_colors-1));

        float rand_val = RandomNum::random_float(0.f, 100.f);

        for (std::size_t i = 0; i < n_star_colors; i++) {
            if (is_in_range(0.f, star_probabilities[i], rand_val)) return i;
            rand_val -= static_cast<float>(star_probabilities[i]);
        }

        return star_probabilities[n_star_colors-1];

    }

}
