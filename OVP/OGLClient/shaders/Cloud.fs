#version 330 core
#include Common.fs

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 Normal;

uniform bool u_ShadowPass;

uniform sampler2D ourTexture;

void main()
{
	vec4 cTex = texture(ourTexture, TexCoord);

	if(u_ShadowPass) {
		cTex.rgb = vec3(0,0,0);
	} else {
		vec3 norm = normalize(Normal);
		float diff = max(dot(norm, -lights[0].dvDirection), 0.0);
		cTex.rgb*=diff;
	}

	color = cTex;
}
