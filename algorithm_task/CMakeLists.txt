cmake_minimum_required(VERSION 3.20)
project(old_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic -O3)

add_executable(solver
    src/main.cpp
    src/solver.cpp
)
target_include_directories(solver PRIVATE src)
