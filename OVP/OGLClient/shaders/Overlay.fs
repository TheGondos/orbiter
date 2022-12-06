#version 330 core

in vec2 uv;
out vec4 color;

uniform sampler2D image;
uniform vec4 color_key;
uniform bool color_keyed;

vec4 textureCK(in sampler2D t, in vec2 uv, in vec2 texSize, in vec2 texelSize)
{
    vec2 f = fract( uv * texSize );
    uv += ( .5 - f ) * texelSize;
    vec4 tl = texture(t, uv);
    if(tl.rgb == color_key.rgb) tl = vec4(0.0);
    vec4 tr = texture(t, uv + vec2(texelSize.x, 0.0));
    if(tr.rgb == color_key.rgb) tr = vec4(0.0);
    vec4 bl = texture(t, uv + vec2(0.0, texelSize.y));
    if(bl.rgb == color_key.rgb) bl = vec4(0.0);
    vec4 br = texture(t, uv + vec2(texelSize.x, texelSize.y));
    if(br.rgb == color_key.rgb) br = vec4(0.0);
    vec4 tA = mix( tl, tr, f.x );
    vec4 tB = mix( bl, br, f.x );
    vec4 ret = mix( tA, tB, f.y );
    ret.rgb /= ret.a;
    return ret;
}


void main()
{
	/*
	if(color_keyed) {
        vec2 ts = textureSize(image, 0);
        vec2 texelSize = 1.0 / ts;
        vec4 sampled = textureCK(image, uv, ts, texelSize);
        color = sampled;
	} else {
		color = texture(image, uv);
	}*/
	vec4 sampled = texture(image, uv);

	if(color_keyed && sampled.rgb == color_key.rgb)
		discard;
	else
		color = sampled;
}
