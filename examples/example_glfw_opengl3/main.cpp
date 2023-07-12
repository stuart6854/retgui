#include <ruic/ruic.hpp>
#include <ruic/elements.hpp>
#include <ruic/internal.hpp>

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

void ruic_opengl_init()
{
    glGenVertexArrays(1, &g_oglDrawData.vao);
    glBindVertexArray(g_oglDrawData.vao);

    glGenBuffers(1, &g_oglDrawData.vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, g_oglDrawData.vertexBuffer);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ruic::DrawVert), (void*)offsetof(ruic::DrawVert, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ruic::DrawVert), (void*)offsetof(ruic::DrawVert, uv));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ruic::DrawVert), (void*)offsetof(ruic::DrawVert, col));

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

    std::uint8_t whitePixel[] = { 255, 255, 255 };

    glGenTextures(1, &g_oglDrawData.whiteTexture);
    glBindTexture(GL_TEXTURE_2D, g_oglDrawData.whiteTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void ruic_opengl3_setup_render_state()
{
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);

    glViewport(0, 0, 1280, 720);
    float L = 0.0f;
    float R = 0.0f + 1280.0f;
    float T = 0.0f;
    float B = 0.0f + 720.0f;
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

void ruic_opengl3_render(const ruic::RuicDrawData* drawData)
{
    const auto vertexSize = drawData->drawList.VertBuffer.size() * sizeof(ruic::DrawVert);
    glBindBuffer(GL_ARRAY_BUFFER, g_oglDrawData.vertexBuffer);
    if (g_oglDrawData.vertexBufferSize < vertexSize)
    {
        glBufferData(GL_ARRAY_BUFFER, vertexSize, nullptr, GL_DYNAMIC_DRAW);
        g_oglDrawData.vertexBufferSize = vertexSize;
    }
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize, drawData->drawList.VertBuffer.data());

    const auto indexSize = drawData->drawList.IdxBuffer.size() * sizeof(ruic::DrawIdx);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_oglDrawData.indexBuffer);
    if (g_oglDrawData.indexBufferSize < indexSize)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize, nullptr, GL_DYNAMIC_DRAW);
        g_oglDrawData.indexBufferSize = indexSize;
    }
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexSize, drawData->drawList.IdxBuffer.data());

    ruic_opengl3_setup_render_state();

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

    auto* window = glfwCreateWindow(1280, 720, "RUIC GLFW+OpenGL3 example", nullptr, nullptr);
    if (window == nullptr)
    {
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // VSync

    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
    {
        return 1;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    ruic::create_context();

    ruic_opengl_init();

    auto element = ruic::create_element<ruic::Element>()
                       ->set_position(ruic::Dim2{ ruic::Dim{ 0.0f, 10.0f }, ruic::Dim{ 0.0f, 10.0f } })
                       ->set_size(ruic::Dim2{ ruic::Dim{ 0.0f, 500.0f }, ruic::Dim{ 0.0f, 300.0f } });
    auto child = ruic::create_element<ruic::Element>()
                     ->set_position(ruic::Dim2{ ruic::Dim(0, 5), ruic::Dim(0, 5) })
                     ->set_size(ruic::Dim2{ ruic::Dim(0.5f, 0), ruic::Dim(0.75f, 0) })
                     ->set_color(ruic::Color(1, 0, 0, 1));
    element->add_child(child);

    ruic::add_to_root(element);

    auto trElement = ruic::create_element<ruic::Element>()
                         ->set_position(ruic::Dim2{ ruic::Dim{ .75f, -5 }, ruic::Dim{ 0, 5 } })
                         ->set_size(ruic::Dim2{ ruic::Dim{ .25f, 0 }, ruic::Dim{ .4f, 0 } })
                         ->set_color(ruic::Color{ 0, 1, 0, 1 });
    ruic::add_to_root(trElement);

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

        ruic::set_root_size(displayWidth, displayHeight);

        ruic::render();
        ruic_opengl3_render(ruic::get_draw_data());

        glfwSwapBuffers(window);
    }

    ruic::destroy_context();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}