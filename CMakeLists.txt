cmake_minimum_required(VERSION 3.10)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(gravity_sim VERSION 1.0)

add_compile_options(-Wall -Wextra -Wpedantic -O3)

file(GLOB_RECURSE Sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/glad/*.c")

add_executable(gravity_sim "${Sources}")

# target_include_directories(GravitySim PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

target_link_libraries(gravity_sim -lm -ldl -lglfw)
