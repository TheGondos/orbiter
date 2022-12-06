struct Light {
    int enabled;
    int dltType;
    vec4 dcvDiffuse;
    vec4 dcvSpecular;
    vec4 dcvAmbient;
    vec3 dvPosition;
    vec3 dvDirection;
    float dvAttenuation0;
    float dvAttenuation1;
    float dvAttenuation2;
    float dvRange;
    float dvFalloff;
    float dvTheta;
    float dvPhi;
};

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 emissive;
    float specular_power;
}; 
