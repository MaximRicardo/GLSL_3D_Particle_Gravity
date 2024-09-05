#pragma once

#include <array>
#include <glm/glm.hpp>

namespace Star {

    constexpr std::size_t n_star_colors = 6;
    extern std::array<glm::vec4, n_star_colors> star_colors;
    extern std::array<float, n_star_colors> star_size_mults;
    extern std::array<float, n_star_colors> star_probabilities;

    std::size_t rand_star_type_idx();

}
