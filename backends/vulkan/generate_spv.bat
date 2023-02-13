@echo off
:: -V: create Vulkan SPIR-V binary
:: -x: save binary output as text-based 32bit hexadecimal numbers
:: -o: output file
glslangValidator -V -x -o vk_shader.vert.u32 vk_shader.vert
glslangValidator -V -x -o vk_shader.frag.u32 vk_shader.frag
pause