cmake_minimum_required(VERSION 3.15)
project(RetGui LANGUAGES CXX)

# Check if RetGui is being used directly or via add_subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(RETGUI_MASTER_PROJECT ON)
else()
    set(RETGUI_MASTER_PROJECT OFF)
endif()

option(RETGUI_BUILD_TESTS "Build test" ${RETGUI_MASTER_PROJECT})

add_subdirectory(retgui)

if(${RETGUI_BUILD_TESTS})
    # SDL
    set(SDL_SHARED OFF CACHE BOOL "Build a SDL shared library (if available)")
    set(SDL_STATIC ON CACHE BOOL "Build a SDL static library (if available)")
    add_subdirectory(external/sdl EXCLUDE_FROM_ALL)

    add_subdirectory(testbed)
endif()