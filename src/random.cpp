#include <cstdlib>
#include <cmath>

#include "random.hpp"

namespace {

    constexpr float PI = 3.141592;

}

namespace RandomNum {

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

}
