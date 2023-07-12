add_executable(ruic_example_glfw_opengl3 main.cpp)

target_include_directories(ruic_example_glfw_opengl3 PRIVATE ../libs/glfw/include)
target_link_libraries(ruic_example_glfw_opengl3 PRIVATE glfw3 glad ruic::ruic)
target_link_directories(ruic_example_glfw_opengl3 PRIVATE ../libs/glfw/lib-vc2022)