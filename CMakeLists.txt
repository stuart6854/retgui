cmake_minimum_required(VERSION 3.15)
project(ruic VERSION 0.0.1 LANGUAGES CXX C)

option(RUIC_BUILD_EXAMPLES "Build the example projects" ON)

add_library(ruic STATIC src/ruic.cpp src/elements.cpp)
add_library(ruic::ruic ALIAS ruic)

target_include_directories(ruic PUBLIC include)

target_compile_features(ruic PRIVATE cxx_std_17)

if (RUIC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()