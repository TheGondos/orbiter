#version 330 core
#include LogDepth.inc

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform vec4 u_Color;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out float Alpha;
out vec4 texCol;
out float flogz;

void main()
{
	gl_Position =  u_ViewProjection * u_Model * vec4(a_Position, 1.0);
    flogz = VS_LOGZ();
	TexCoord = a_TexCoord;
    FragPos = vec3(u_Model * vec4(a_Position, 1.0));
//    Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    texCol = u_Color;
}
