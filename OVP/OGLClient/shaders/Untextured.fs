#version 330 core

out vec4 color;

uniform vec3 quad_color;

void main()
{
	color = vec4(quad_color,1);
}
