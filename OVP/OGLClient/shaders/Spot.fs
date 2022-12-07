#version 330 core

layout(location = 0) out vec4 color;
in vec2 TexCoord;

uniform vec3 u_SpotColor;

uniform sampler2D ourTexture;

void main()
{
	//vec4 c = texture(ourTexture, TexCoord);
	color = texture(ourTexture, TexCoord) * vec4(u_SpotColor,1.0);
    //color = vec4(0.5,0.5,0.5,0.5);
}
