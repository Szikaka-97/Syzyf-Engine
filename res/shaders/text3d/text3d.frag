#version 460

#pragma transparent
#pragma no_cull

in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

#include "shared/shared.h";
#include "shared/uniforms.h";

out vec4 fragColor;

uniform sampler2D fontAtlas;
uniform vec4 textColor;
uniform float pxRange;
uniform bool useMsdf;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    if (useMsdf) {
        vec3 msd = texture(fontAtlas, ps_in.texcoords).rgb;
        float sd = median(msd.r, msd.g, msd.b);
        
        vec2 texSize = vec2(textureSize(fontAtlas, 0));
        vec2 dx = dFdx(ps_in.texcoords) * texSize.x;
        vec2 dy = dFdy(ps_in.texcoords) * texSize.y;
        float toPixels = pxRange * inversesqrt(dot(dx, dx) + dot(dy, dy));
        
        float alpha = clamp(toPixels * (sd - 0.5) + 0.5, 0.0, 1.0);

        if (alpha < 0.001) discard;
        
        fragColor = vec4(textColor.rgb, textColor.a * alpha);
    } else {
        float alpha = texture(fontAtlas, ps_in.texcoords).r;
        if (alpha < 0.5) discard;
        fragColor = vec4(textColor.rgb, textColor.a);
    }
}
