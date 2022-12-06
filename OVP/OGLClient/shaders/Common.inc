
#define LIGHT_POINT 0
#define LIGHT_SPOT  1
#define LIGHT_DIRECTIONAL 2
#define NR_LIGHTS 8

#define saturate(x) clamp(x, 0.0, 1.0)

#include Types.inc
#include Uniforms.inc

void CalcLight( out vec3 diffuse, out vec3 specular,
                in Light light, in vec3 normal, in vec3 fragPos, in vec3 viewDir, in float spec_power)
{
    if(light.enabled != 1) {
        diffuse=vec3(0);
        specular=vec3(0);
        return;
    }
    vec3 lightDir;
    if(light.dltType == LIGHT_DIRECTIONAL) {
        lightDir = normalize(-light.dvDirection);
    } else {
        lightDir = normalize(light.dvPosition - fragPos);
    }

    float spotfactor = 1.0;
    if(light.dltType == LIGHT_SPOT) {
        spotfactor = (dot(lightDir, normalize(-light.dvDirection)) - light.dvPhi) * light.dvTheta;
        spotfactor = clamp(spotfactor, 0.0, 1.0);
    }

    // diffuse shading
    float diff = clamp(dot(normal, lightDir), 0.0, 1.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0;
    if (spec_power != 0 && diff > 0) {
        spec = pow(clamp(dot(viewDir, reflectDir), 0.0, 1.0), spec_power);
    }

    float attenuation = 1.0;
    if(light.dltType != LIGHT_DIRECTIONAL) {
        // attenuation
        float distance    = length(light.dvPosition - fragPos);
        attenuation = 1.0 / (light.dvAttenuation0 + light.dvAttenuation1 * distance +
                    light.dvAttenuation2 * (distance * distance));
    }

    diff*=attenuation*spotfactor;
    spec*=attenuation*spotfactor;

    // combine results
    diffuse  = light.dcvDiffuse.rgb * diff;
    specular = light.dcvDiffuse.rgb * spec;
    //specular = light.dcvSpecular.rgb * spec;
} 

vec3 lumaBasedReinhardToneMapping(vec3 color)
{
	//const float gamma=2.2;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = 1.3 * luma / (1. + luma);
	color *= toneMappedLuma / luma;
	//color = pow(color, vec3(1. / gamma));
	return color;
}
