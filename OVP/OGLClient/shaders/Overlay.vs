#version 330 core

in vec4 vertex;
uniform mat4 projection;
out vec2 uv;

void main()
{
     gl_Position = projection * vec4(vertex.zw, 0.0, 1.0);
	uv = vertex.xy;
}
