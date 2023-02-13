#version 450 core

layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(push_constant) uniform PushConstants {
    vec2 scale;
    vec2 translate;
} push_consts;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec4 out_color;

void main()
{
    gl_Position = vec4(a_pos * push_consts.scale + push_consts.translate, 0.0, 1.0);
    out_uv = a_uv;
    out_color = a_color;
}