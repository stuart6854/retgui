#include <retgui/retgui.hpp>
#include <retgui/elements.hpp>
#include <retgui/io.hpp>
#include <retgui/internal.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <string>
#include <cstddef>
#include <cstdint>
#include <iostream>

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void glfwCursorPosCallback(GLFWwindow* window, double x, double y)
{
    auto& io = retgui::get_current_context()->io;
    io.cursorPos = { float(x), float(y) };
}

void glfwMouseBtnCallback(GLFWwindow* window, int btn, int action, int mods)
{
    if (action == GLFW_REPEAT)
    {
        return;
    }

    auto& io = retgui::get_current_context()->io;
    io.mouseBtns[btn] = action == GLFW_PRESS;
}

void APIENTRY
glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " << message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY: std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION: std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER: std::cout << "Source: Other"; break;
    }
    std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY: std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE: std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER: std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP: std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP: std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER: std::cout << "Type: Other"; break;
    }
    std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH: std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM: std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW: std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    }
    std::cout << std::endl;
    std::cout << std::endl;
}

const char* OGLVertexShaderSrc = R"(
#version 130

in vec2 in_position;
in vec2 in_texCoord;
in vec4 in_color;

out vec2 frag_texCoord;
out vec4 frag_color;

uniform mat4 u_projMatrix;

void main()
{
    frag_texCoord = in_texCoord;
    frag_color = in_color;
    gl_Position = u_projMatrix * vec4(in_position.xy, 0.0, 1.0);
}
)";

const char* OGLFragmentShaderSrc = R"(
#version 130

in vec2 frag_texCoord;
in vec4 frag_color;

out vec4 out_fragColor;

uniform sampler2D u_texture;

void main()
{
    out_fragColor = frag_color * texture(u_texture, frag_texCoord.st);
}
)";

struct DrawData
{
    GLuint vao;
    GLuint vertexBuffer;
    std::size_t vertexBufferSize;
    GLuint indexBuffer;
    std::size_t indexBufferSize;

    GLuint program;

    GLuint whiteTexture;
};
static DrawData g_oglDrawData{};

void retgui_opengl_init()
{
    glGenVertexArrays(1, &g_oglDrawData.vao);
    glBindVertexArray(g_oglDrawData.vao);

    glGenBuffers(1, &g_oglDrawData.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_oglDrawData.vertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(retgui::DrawVert), (void*)offsetof(retgui::DrawVert, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(retgui::DrawVert), (void*)offsetof(retgui::DrawVert, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(retgui::DrawVert), (void*)offsetof(retgui::DrawVert, col));

    glGenBuffers(1, &g_oglDrawData.indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_oglDrawData.indexBuffer);

    glBindVertexArray(0);

    auto vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &OGLVertexShaderSrc, nullptr);
    glCompileShader(vertShader);

    {
        int success;
        char infoLog[512];
        glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(vertShader, 512, nullptr, infoLog);
            fprintf(stderr, "Vertex Shader: Error while compiling shader: %s", infoLog);
        }
    }

    auto fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &OGLFragmentShaderSrc, nullptr);
    glCompileShader(fragShader);

    {
        int success;
        char infoLog[512];
        glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(fragShader, 512, nullptr, infoLog);
            fprintf(stderr, "Fragment Shader: Error while compiling shader: %s", infoLog);
        }
    }

    g_oglDrawData.program = glCreateProgram();
    glAttachShader(g_oglDrawData.program, vertShader);
    glAttachShader(g_oglDrawData.program, fragShader);
    glLinkProgram(g_oglDrawData.program);

    {
        int success;
        char infoLog[512];
        glGetProgramiv(g_oglDrawData.program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(g_oglDrawData.program, 512, NULL, infoLog);
            fprintf(stderr, "Program: Error while linking shaders: %s", infoLog);
        }
    }

    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

#if 0
    std::uint8_t whitePixel[] = { 255, 255, 255 };

    glGenTextures(1, &g_oglDrawData.whiteTexture);
    glBindTexture(GL_TEXTURE_2D, g_oglDrawData.whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
#endif

    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::uint32_t> textureData{};
    retgui::get_current_context()->io.Fonts.get_texture_data_as_rgba32(textureData, width, height);

    glGenTextures(1, &g_oglDrawData.whiteTexture);
    glBindTexture(GL_TEXTURE_2D, g_oglDrawData.whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData.data());

    glBindTexture(GL_TEXTURE_2D, 0);
}

void retgui_opengl3_setup_render_state()
{
    auto* context = retgui::get_current_context();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    glViewport(0, 0, context->displaySize.x, context->displaySize.y);
    float L = 0.0f;
    float R = 0.0f + context->displaySize.x;
    float T = 0.0f;
    float B = 0.0f + context->displaySize.y;
    const float orthoProjection[4][4] = {
        { 2.0f / (R - L), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (T - B), 0.0f, 0.0f },
        { 0.0, 0.0f, -1.0f, 0.0f },
        { (R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f },
    };

    glUseProgram(g_oglDrawData.program);
    glUniformMatrix4fv(glGetUniformLocation(g_oglDrawData.program, "u_projMatrix"), 1, GL_FALSE, &orthoProjection[0][0]);

    glBindVertexArray(g_oglDrawData.vao);
}

void retgui_opengl3_render(const retgui::RetGuiDrawData* drawData)
{
    const auto vertexSize = drawData->drawList.VertBuffer.size() * sizeof(retgui::DrawVert);
    glBindBuffer(GL_ARRAY_BUFFER, g_oglDrawData.vertexBuffer);
    if (g_oglDrawData.vertexBufferSize < vertexSize)
    {
        glBufferData(GL_ARRAY_BUFFER, vertexSize, nullptr, GL_DYNAMIC_DRAW);
        g_oglDrawData.vertexBufferSize = vertexSize;
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, drawData->drawList.VertBuffer.data());

    const auto indexSize = drawData->drawList.IdxBuffer.size() * sizeof(retgui::DrawIdx);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_oglDrawData.indexBuffer);
    if (g_oglDrawData.indexBufferSize < indexSize)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, nullptr, GL_DYNAMIC_DRAW);
        g_oglDrawData.indexBufferSize = indexSize;
    }
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexSize, drawData->drawList.IdxBuffer.data());

    retgui_opengl3_setup_render_state();

    glBindTexture(GL_TEXTURE_2D, g_oglDrawData.whiteTexture);

    glDrawElements(GL_TRIANGLES, drawData->drawList.IdxBuffer.size(), GL_UNSIGNED_INT, nullptr);
}

int main(int argc, char** argv)
{
    glfwSetErrorCallback(error_callback);

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    auto* window = glfwCreateWindow(1280, 720, "RetGui GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
    {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // VSync

    glfwSetCursorPosCallback(window, glfwCursorPosCallback);
    glfwSetMouseButtonCallback(window, glfwMouseBtnCallback);

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        return 1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    retgui::create_context();

    auto& io = retgui::get_current_context()->io;
    auto* font32 = io.Fonts.add_font_from_file("fonts/Karla-Regular.ttf", 32.0f);
    auto* font64 = io.Fonts.add_font_from_file("fonts/Karla-Regular.ttf", 64.0f);

    retgui_opengl_init();

    auto element = retgui::create_element<retgui::Element>();
    element->set_position(retgui::Dim2{ retgui::Dim{ 0.0f, 10.0f }, retgui::Dim{ 0.0f, 10.0f } });
    element->set_size(retgui::Dim2{ retgui::Dim{ 0.0f, 500.0f }, retgui::Dim{ 0.0f, 300.0f } });
    retgui::add_to_root(element);

    auto child = retgui::create_element<retgui::Element>();
    child->set_position(retgui::Dim2{ retgui::Dim(0, 5), retgui::Dim(0, 5) });
    child->set_size(retgui::Dim2{ retgui::Dim(0.5f, 0), retgui::Dim(0.75f, 0) });
    child->set_color(retgui::Color(1, 0, 0, 1));
    element->add_child(child);

    auto buttonChild = retgui::create_element<retgui::Button>();
    buttonChild->set_position(retgui::Dim2{ retgui::Dim(1.0f, -125), retgui::Dim(0.0f, 5) });
    buttonChild->set_size(retgui::Dim2{ retgui::Dim(0, 120), retgui::Dim(0, 30) });
    buttonChild->set_color(retgui::Color(0.450f, 0.418f, 0.418f, 1));
    buttonChild->set_hovered_color(retgui::Color(0.431f, 0.401f, 0.401f, 1));
    buttonChild->set_active_color(retgui::Color(0.641f, 0.426f, 0.426f, 1));
    buttonChild->set_on_clicked([]() { std::cout << "Button Clicked!" << std::endl; });
    element->add_child(buttonChild);

    auto btnLabel = retgui::create_element<retgui::Label>();
    btnLabel->set_position(retgui::Dim2{ retgui::Dim(0, 0), retgui::Dim(0, 0) });
    btnLabel->set_size(retgui::Dim2{ retgui::Dim(1, 0), retgui::Dim(1, 0) });
    btnLabel->set_color(retgui::Color(1, 0, 1, 1));
    btnLabel->set_font(font32);
    btnLabel->set_text("Hello World!");
    buttonChild->add_child(btnLabel);

    auto trElement = retgui::create_element<retgui::Element>();
    trElement->set_position(retgui::Dim2{ retgui::Dim{ .75f, -5 }, retgui::Dim{ 0, 5 } });
    trElement->set_size(retgui::Dim2{ retgui::Dim{ .25f, 0 }, retgui::Dim{ .4f, 0 } });
    trElement->set_color(retgui::Color{ 0, 1, 0, 1 });
    retgui::add_to_root(trElement);

    auto someLabel = retgui::create_element<retgui::Label>();
    someLabel->set_position(retgui::Dim2{ retgui::Dim(0, 0), retgui::Dim(0.5f, 0) });
    someLabel->set_size(retgui::Dim2{ retgui::Dim(1, 0), retgui::Dim(0, 30) });
    someLabel->set_color(retgui::Color(1, 1, 1, 1));
    someLabel->set_font(font32);
    someLabel->set_text(
        "abcdefghijklmnopqrstuvqxyz ABCDEFGHIJKLMNOPQRSTUVQXYZ\n1234567890.:,;'\"(!?)+-*/=\nThe quick brown fox jumps over the lazy dog. "
        "1234567890");
    retgui::add_to_root(someLabel);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        std::int32_t displayWidth{};
        std::int32_t displayHeight{};
        glfwGetFramebufferSize(window, &displayWidth, &displayHeight);

        retgui::set_root_size(displayWidth, displayHeight);

        retgui::update();

        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        retgui::render();
        retgui_opengl3_render(retgui::get_draw_data());

        glfwSwapBuffers(window);
    }

    retgui::destroy_context();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}