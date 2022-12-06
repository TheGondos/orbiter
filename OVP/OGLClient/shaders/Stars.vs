#version 330 core

layout(location = 0) in vec4 a_Position;

uniform mat4 u_ViewProjection;

out vec4 starColor;

void main()
{
	gl_Position = u_ViewProjection * vec4(a_Position.xyz, 1.0);
	starColor = vec4(a_Position.w,a_Position.w,a_Position.w,1.0);
}
