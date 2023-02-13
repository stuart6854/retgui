#version 450 core

layout(location = 0) in vec2 in_uv;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 frag_color;

void main()
{
    frag_color = in_color;
}