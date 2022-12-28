#version 330 core
#include LogDepth.inc

layout(location = 0) out vec4 color;
in vec2 TexCoord;
in float flogz;

uniform vec3 u_SpotColor;

uniform sampler2D colorTexture;

void main()
{
	//vec4 c = texture(colorTexture, TexCoord);
	color = texture(colorTexture, TexCoord) * vec4(u_SpotColor,1.0);
	gl_FragDepth = FS_LOGZ(flogz);
    //color = vec4(0.5,0.5,0.5,0.5);
}
