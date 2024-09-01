#include <vector>
#include <cmath>

#include "sphere.hpp"

constexpr float deg_to_rad = 0.0174532925;

namespace Sphere {

    std::vector<float> sphere_vertices() {

        std::vector<float> vertices;

        unsigned rings = 10;
        unsigned slices = 10;

        for (unsigned i = 0; i < (rings+2); i++) {
            for (unsigned j = 0; j < slices; j++) {
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::sin(deg_to_rad*(360.f*j/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*i)));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::cos(deg_to_rad*(360.f*j/slices)));

                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::sin(deg_to_rad*(360.f*(j+1)/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1))));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::cos(deg_to_rad*(360.f*(j+1)/slices)));
                
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::sin(deg_to_rad*(360.f*j/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1))));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::cos(deg_to_rad*(360.f*j/slices)));


                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::sin(deg_to_rad*(360.f*j/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*i)));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::cos(deg_to_rad*(360.f*j/slices)));
                
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::sin(deg_to_rad*(360.f*(j+1)/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*i)));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*i))*std::cos(deg_to_rad*(360.f*(j+1)/slices)));

                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::sin(deg_to_rad*(360.f*(j+1)/slices)));
                vertices.push_back(std::sin(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1))));
                vertices.push_back(std::cos(deg_to_rad*(270.f+(180.f/(rings+1))*(i+1)))*std::cos(deg_to_rad*(360.f*(j+1)/slices)));
            }
        }

        return vertices;

    }

}
