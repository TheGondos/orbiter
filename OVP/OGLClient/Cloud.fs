#version 330 core

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 Normal;

uniform vec3 u_SunDir;
uniform bool u_ShadowPass;

uniform sampler2D ourTexture;

void main()
{
	vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -u_SunDir), 0.0);
//    vec4 diffuse = (diff * u_Material.diffuse);

	if(u_ShadowPass)
		color = vec4(vec3(0,0,0), texture(ourTexture, TexCoord).a);
	else
		color = vec4(diff * texture(ourTexture, TexCoord).rgb, texture(ourTexture, TexCoord).a);
	//color = vec4(1.0,1.0,0.5,1.0);
}
