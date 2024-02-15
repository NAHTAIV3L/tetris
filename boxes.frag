#version 330 core

uniform sampler2D image;
uniform float time;

in vec4 out_color;
in vec2 out_uv;

void main()
{

    gl_FragColor = out_color;
}
