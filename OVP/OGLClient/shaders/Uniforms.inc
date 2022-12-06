uniform int u_lighting;
uniform Light lights[NR_LIGHTS];

uniform Material u_Material;

uniform vec4 u_FogColor;
uniform float u_FogDensity;

uniform sampler2D ourTexture;
uniform bool u_Textured;
uniform bool u_ModulateAlpha;
uniform float u_MatAlpha;

uniform vec3 u_ViewPos;
uniform vec3 u_bgcol;
