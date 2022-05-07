#version 330 core
in vec2 TexCoords;
layout(location = 0) out vec4 color;

uniform sampler2D image;
uniform vec3 spriteColor;

void main()
{    
    color = vec4(spriteColor, 1.0) * texture(image, TexCoords);
	color=vec4(1.0,1.0,0.5,1.0);
}
