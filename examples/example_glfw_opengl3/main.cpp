#include <ruic/ruic.hpp>

#include <GLFW/glfw3.h>

#include <cstdint>

int main(int argc, char** argv)
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    auto* window = glfwCreateWindow(1280, 720, "RUIC GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
    {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // VSync

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // #TODO: Update here

        std::int32_t displayWidth{};
        std::int32_t displayHeight{};
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // #TODO: Render here

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}