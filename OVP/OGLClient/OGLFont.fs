#version 330 core

in vec2 uv;
out vec4 color;

uniform sampler2D font;
uniform vec3 font_color;

void main()
{
	vec4 sampled = vec4(1.0, 1.0, 1.0, texture(font, uv).r);
	color = vec4(font_color, 1.0) * sampled;
//	color = vec4(1.0,0.5,1.0, 0.5);
}
