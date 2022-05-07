#version 330 core

in vec2 uv;
out vec4 color;

uniform sampler2D image;
uniform vec4 color_key;
uniform bool color_keyed;
//uniform vec3 font_color;

void main()
{
	vec4 sampled = texture(image, uv);// vec4(1.0, 1.0, 1.0, texture(font, uv).r);
	if(color_keyed && sampled.rgb == color_key.rgb) discard;
	color = sampled;
//	color = vec4(1.0,0.5,1.0, 0.5);
}
