cmake_minimum_required(VERSION 3.15)
project(RetGui VERSION 0.0.1 LANGUAGES CXX C)

option(RETGUI_BUILD_EXAMPLES "Build the example projects" ON)

add_library(RetGui STATIC src/retgui.cpp src/types.cpp src/elements.cpp src/io.cpp src/fonts.cpp)
add_library(RetGui::RetGui ALIAS RetGui)

target_include_directories(RetGui PRIVATE src PUBLIC include)

set_target_properties(RetGui PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED TRUE)

if (RETGUI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()