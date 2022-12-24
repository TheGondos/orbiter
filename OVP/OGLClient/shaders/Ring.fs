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
    vec4 cTex = vec4(1.0, 1.0, 1.0, u_MatAlpha);
    vec3 Base = (u_Material.ambient.rgb * lights[0].dcvAmbient.rgb) + u_Material.emissive.rgb;
    vec4 cSpec = u_Material.specular;

    if(u_Textured)
	    cTex = texture(colorTexture, TexCoord);

    if(u_ModulateAlpha)
        cTex.a *= u_Material.diffuse.a;

    vec3 cDiffLocal = vec3(0);
    vec3 cSpecLocal = vec3(0);
    if(u_lighting == 1) {
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize( - FragPos);

        for(int i = 0; i < NR_LIGHTS; i++) {
            vec3 diffuse;
            vec3 specular;
            CalcLight(diffuse, specular,
                      lights[i], norm, FragPos, viewDir, u_Material.specular_power);

            cDiffLocal += diffuse;
            if(u_Material.specular_power != 0)
                cSpecLocal += specular;
        }
    }
    cTex.rgb *= Base + u_Material.diffuse.rgb * cDiffLocal;
    cSpec.rgb *= cSpecLocal;
    
    cTex.rgb += cSpec.rgb;

	color.rgb = lumaBasedReinhardToneMapping(cTex.rgb);
	color.a = cTex.a;
//    color = min(cTex,1);
	gl_FragDepth = FS_LOGZ(flogz);










/*
    // ambient
    vec4 ambient = u_Material.ambient * 0.1;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -u_SunDir), 0.0);
    vec4 diffuse = (diff * u_Material.diffuse);
    
    // specular
    vec3 viewDir = normalize( - FragPos);
    vec3 reflectDir = reflect(u_SunDir, norm);  

    vec4 specular = vec4(0,0,0,0);
    if (u_Material.specular_power != 0) {
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), u_Material.specular_power);
        specular = (spec * u_Material.specular);  
    }

	// emissive
	vec4 emissive = u_Material.emissive;

    vec4 result = ambient + diffuse + emissive + specular;

	vec4 text = texture(colorTexture, TexCoord);
    if(!u_Textured)
        text=vec4(1,1,1,1);

    float alpha = u_MatAlpha;
    //float alpha = u_Material.diffuse.a;
    if(u_Textured)
        alpha=text.a;

    if(u_ModulateAlpha)
        alpha = u_Material.diffuse.a;
    
//    vec4 tmpout = vec4(vec3(result),alpha) * text;
    vec4 tmpout = vec4(vec3(0),alpha) * text;
    if(u_lighting == 1) {
        for(int i = 0; i < NR_LIGHTS; i++)
            tmpout.rgb += CalcLight(lights[i], norm, FragPos, viewDir);
    }
    color = clamp(tmpout,0.0,1.0);
    */
}
