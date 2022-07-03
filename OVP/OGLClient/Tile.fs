#version 330 core

layout(location = 0) out vec4 color;

in vec2 TexCoord;
in vec3 Normal;

uniform vec3 u_SunDir;
uniform vec3 u_bgcol;
uniform sampler2D ourTexture;
uniform vec4 u_FogColor;
uniform float u_FogDensity;
in vec4 eyeSpacePosition;

void main()
{
	vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -u_SunDir), 0.0);
//    vec4 diffuse = (diff * u_Material.diffuse);

	color = vec4(diff * texture(ourTexture, TexCoord).rgb, texture(ourTexture, TexCoord).a) + vec4(u_bgcol,0);

	if(u_FogDensity != 0.0) {
		float fogCoordinate = abs(eyeSpacePosition.z / eyeSpacePosition.w);
		float fogfactor = exp(-u_FogDensity * fogCoordinate);
		fogfactor = 1.0 - clamp(fogfactor, 0.0, 1.0);
		color = vec4(mix(color, u_FogColor, fogfactor).rgb, color.a);
	}

	//color = vec4(fogfactor,fogfactor,fogfactor,1.0);

	//color = mix(color, u_FogColor, fogfactor);
	//color = vec4(1.0,1.0,0.5,1.0);
}
