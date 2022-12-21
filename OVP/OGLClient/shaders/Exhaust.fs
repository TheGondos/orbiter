#version 330 core

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in float Alpha;
in vec4 texCol;
uniform sampler2D colorTexture;

void main()
{
    vec4 tex = texture(colorTexture, TexCoord);
//    vec4 c = texCol;
  //  c.a = Alpha;

//    color = tex * c;
    //color = vec4(1,1,1,1);
    color = tex;
}
