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

#include "shared/shared.h"
#include "shared/uniforms.h"

uniform float uOpacity;
uniform float uIntensity;
uniform vec2 uSpeed;
uniform sampler2D uNoiseTexture;
uniform sampler2D uSubtractionTexture;
uniform sampler2D uColorGradientTexture;

out vec4 fragColor;

void main() {
    float noise = texture(uNoiseTexture, ps_in.texcoords + Global_Time * uSpeed).r;
    float subtraction = texture(uSubtractionTexture, ps_in.texcoords).r;
    noise = clamp(noise, 0.0, 1.0);

    vec4 colorGradient = texture(uColorGradientTexture, vec2(noise));

    const vec3 color = colorGradient.rgb * uIntensity;
    const float alpha = colorGradient.a * uOpacity * clamp(1.0 - subtraction, 0.0, 1.0);

    fragColor = vec4(color, alpha);
}
