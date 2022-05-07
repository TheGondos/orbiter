#version 330 core

layout(location = 0) out vec4 color;

//uniform vec4 u_Color;
in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
uniform vec3 u_SunDir;
uniform sampler2D ourTexture;
uniform bool u_Textured;
uniform float u_MatAlpha;

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float specular_power;
}; 

uniform Material u_Material;
uniform vec3 u_ViewPos;
void main()
{
	// ambient
    vec4 ambient = u_Material.ambient * 0.02;
  	
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

//    vec4 result = ambient * u_Material.ambient.a + diffuse * u_Material.diffuse.a + specular * u_Material.specular.a + emissive * u_Material.emissive.a;
//    vec4 result = ambient  + diffuse + specular + emissive ;
    vec4 result = ambient + diffuse + emissive + specular;

	vec4 text = texture(ourTexture, TexCoord);
//    float alpha = u_Material.ambient.a;
    if(!u_Textured)
        text=vec4(1,1,1,1);

    float alpha = u_MatAlpha;
//    float alpha = u_Material.diffuse.a;
//    if(u_Textured)
  //      alpha=text.a;
    //if(text.a==0)
      // text.a=0.1;
    
    color = vec4(vec3(result),alpha) * text;
//    color = vec4(vec3(result),u_Material.ambient.a) * text;
    //color = vec4(vec3(result),u_Material.ambient.a);
    //color = vec4(vec3(result),0.5) * text;
  //  color = vec4(vec3(result),u_Material.diffuse.a) * text;
//    color=vec4(1.0,1.0,1.0,1.0);
//    color = vec4(vec3(result),1.0) * text;
    //color = result * text;
}
