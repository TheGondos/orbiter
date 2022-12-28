#version 330 core
#include Common.inc
#include LogDepth.inc

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in float flogz;

void main()
{
    vec3 Base = (u_Material.ambient.rgb * lights[0].dcvAmbient.rgb) + u_Material.emissive.rgb;
    vec4 cSpec = u_Material.specular;

	vec4 cTex = texture(colorTexture, TexCoord);

    cTex.rgb *= Base;

	color.rgb = lumaBasedReinhardToneMapping(cTex.rgb);
	color.a = cTex.a;
//    color = min(cTex,1);
	gl_FragDepth = FS_LOGZ(flogz);

}
