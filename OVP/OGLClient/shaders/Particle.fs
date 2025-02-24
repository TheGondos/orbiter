#version 330 core
#include Common.inc
#include LogDepth.inc

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in float Alpha;
in vec4 texCol;
in float flogz;
//uniform sampler2D colorTexture;
//uniform vec4 u_Color;

void main()
{
    vec4 tex = texture(colorTexture, TexCoord);
    vec4 c = texCol;
    c.a = Alpha;

    color = tex * c;
	gl_FragDepth = FS_LOGZ(flogz);
}
