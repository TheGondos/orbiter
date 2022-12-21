#version 330 core
#include Common.inc

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

in vec4 eyeSpacePosition;

void main()
{
	vec4 cTex = texture(colorTexture, TexCoord);
    if(u_lighting == 1) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize( - FragPos);

        vec3 l = vec3(0.0);
        for(int i = 0; i < NR_LIGHTS; i++) {
            vec3 diffuse;
            vec3 specular;
            CalcLight(diffuse, specular,
                      lights[i], norm, FragPos, viewDir, u_Material.specular_power);

            l+= diffuse;
            if(u_Material.specular_power!=0)
                l+=specular;
        }

        cTex.rgb*=l;//clamp(l, 0.0, 1.0);
    }
	cTex.rgb += u_bgcol;

	if(u_FogDensity != 0.0) {
		float fogCoordinate = abs(eyeSpacePosition.z / eyeSpacePosition.w);
		float fogfactor = exp(-u_FogDensity * fogCoordinate);
		fogfactor = 1.0 - clamp(fogfactor, 0.0, 1.0);
		cTex.rgb = mix(cTex, u_FogColor, fogfactor).rgb;
	}

	color.rgb = lumaBasedReinhardToneMapping(cTex.rgb);
	color.a = cTex.a;
//    color = min(cTex,1);


	/*
	vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -u_SunDir), 0.0);
//    vec4 diffuse = (diff * u_Material.diffuse);

	color = vec4(diff * texture(colorTexture, TexCoord).rgb, texture(colorTexture, TexCoord).a) + vec4(u_bgcol,0);

	if(u_FogDensity != 0.0) {
		float fogCoordinate = abs(eyeSpacePosition.z / eyeSpacePosition.w);
		float fogfactor = exp(-u_FogDensity * fogCoordinate);
		fogfactor = 1.0 - clamp(fogfactor, 0.0, 1.0);
		color = vec4(mix(color, u_FogColor, fogfactor).rgb, color.a);
	}

	//color = vec4(fogfactor,fogfactor,fogfactor,1.0);

	//color = mix(color, u_FogColor, fogfactor);
	//color = vec4(1.0,1.0,0.5,1.0);
	*/
}
