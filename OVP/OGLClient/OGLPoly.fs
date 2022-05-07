#version 330 core

out vec4 color;
in float dist;

uniform vec3 line_color;

void main()
{
//	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(font, uv).r);
	if(fract(dist)>0.3)
		discard;
	else
		color = vec4(line_color, 1.0);// * fract(dist);// * sampled;
}
