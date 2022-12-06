#version 330 core

layout(location = 0) out vec4 color;

uniform float u_ShadowDepth;

void main()
{
    color = vec4(0,0,0,u_ShadowDepth);
}
