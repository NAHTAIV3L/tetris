#version 330 core

uniform vec2 res;

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

out vec4 out_color;
out vec2 out_uv;

vec2 project(vec2 point)
{
    return (((2.0 * point) / res) - vec2(1)) * vec2(1.0, -1.0);
}

void main()
{
    out_uv = uv;
    out_color = color;
    gl_Position = vec4(project(pos), 0.0, 1.0);
}
