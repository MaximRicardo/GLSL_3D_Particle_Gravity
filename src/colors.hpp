#pragma once

#include <array>
#include <glm/glm.hpp>

namespace Colors {

    constexpr std::size_t n_star_colors = 6;
    extern std::array<glm::vec4, n_star_colors> star_colors;

    std::size_t rand_star_color_idx();

}
