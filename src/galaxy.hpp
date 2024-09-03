#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Galaxy {

    std::vector<glm::vec4> generate_galaxy(std::size_t n_points, float radius);

}
