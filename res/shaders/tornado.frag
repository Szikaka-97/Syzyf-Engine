#version 460

#pragma oit_transparent

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

layout (location = 0) out vec4 accumValue;
layout (location = 1) out float revealValue;

float calcWeight(float alpha) {
	return clamp(
		pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
		0.001,
		3000.0
	);
}

void main() {
    float noise = texture(uNoiseTexture, ps_in.texcoords + Global_Time * uSpeed).r;
    float subtraction = texture(uSubtractionTexture, ps_in.texcoords).r;
    noise = clamp(noise, 0.0, 1.0);

    vec4 colorGradient = texture(uColorGradientTexture, vec2(noise));

    const vec3 color = colorGradient.rgb * uIntensity;
    const float alpha = colorGradient.a * uOpacity * clamp(1.0 - subtraction, 0.0, 1.0);

    const float weight = calcWeight(alpha);
    const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);
    accumValue = vec4(color * dot(viewDir, normalize(ps_in.normal)) * alpha, alpha) * weight;
    revealValue = alpha;
}
