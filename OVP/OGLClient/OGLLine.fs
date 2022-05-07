#version 330 core

out vec4 color;
in float dist;

uniform vec4 line_color;

void main()
{
	if(fract(dist)>0.3)
		discard;
	else
		color = vec4(line_color);
}
