#version 330 core

in vec3 vertex;
out float dist;
uniform mat4 projection;

void main()
{
     gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
     dist = vertex.z;
}
