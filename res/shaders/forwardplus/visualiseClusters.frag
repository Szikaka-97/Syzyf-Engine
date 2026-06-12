#version 460

in VS_OUT {
	vec3 worldPos;
	flat uint index;
	flat uint lightsAmount;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

#pragma oit_transparent
#pragma no_cull
// #pragma ignore_depth

uniform float opacity;

layout (location = 0) out vec4 accumValue;
layout (location = 1) out float revealValue;

float calcWeight(float alpha) {
	return clamp(
		pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
		0.001,
		3000.0
	);
}

vec3 calcColor(float factor) {
	if (factor < 0.5) {
		return mix(vec3(0, 0, 1), vec3(0, 1, 0), factor * 2);
	}

	return mix(vec3(0, 1, 1), vec3(1, 0, 0), (factor - 0.5) * 2);
}

void main() {
	const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);

	float weight = clamp(float(ps_in.lightsAmount) / 50, 0, 1);

	if (weight < 0.1) {
		discard;
	}

	const vec3 color = calcColor(weight);

	// FragColor = vec4(color, opacity * weight);

	const float alpha = opacity * weight;

	const float blendWeight = calcWeight(alpha);

	accumValue = vec4(color * alpha, alpha) * blendWeight;

	revealValue = alpha;

}