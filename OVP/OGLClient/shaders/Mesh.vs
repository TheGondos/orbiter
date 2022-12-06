#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

void main()
{
	gl_Position =  u_ViewProjection * u_Model * vec4(a_Position, 1.0);

   	TexCoord = a_TexCoord;
    FragPos = vec3(u_Model * vec4(a_Position, 1.0));
    Normal = mat3(transpose(inverse(u_Model))) * a_Normal;  
}
