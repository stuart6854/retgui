add_executable(RetGui_Example_GLFW_OpenGL3 main.cpp)

target_include_directories(RetGui_Example_GLFW_OpenGL3 PRIVATE ../libs/glfw/include)
target_link_libraries(RetGui_Example_GLFW_OpenGL3 PRIVATE glfw3 glad RetGui::RetGui)
target_link_directories(RetGui_Example_GLFW_OpenGL3 PRIVATE ../libs/glfw/lib-vc2022)