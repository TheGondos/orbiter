#version 330 core

out vec4 color;

//uniform vec4 u_Color;
in vec4 starColor;

void main()
{
	color = starColor;//vec4(1.0,1.0,1.0,1.0);//u_Color;
}
