#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_Tangent;
layout(location = 3) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
out vec2 TexCoord;

void main()
{
	gl_Position =  u_ViewProjection * u_Model * vec4(a_Position, 1.0);
//	TexCoord = vec2(a_TexCoord.x, 1.0 - a_TexCoord.y);
	TexCoord = a_TexCoord;
}
