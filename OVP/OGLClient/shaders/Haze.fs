#version 330 core

layout(location = 0) out vec4 color;

//uniform vec4 u_Color;
in vec2 TexCoord;
in vec4 col;

uniform sampler2D colorTexture;

void main()
{
	vec4 c = texture(colorTexture, TexCoord);
	color = texture(colorTexture, TexCoord) * col;
	//color=vec4(1.0,0.0,0.0,1.0);
}
