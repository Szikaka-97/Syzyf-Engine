#version 460

in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

uniform vec4 uColor;

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
	const float weight = calcWeight(uColor.a);

	const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);

	accumValue = vec4(uColor.rgb * dot(viewDir, normalize(ps_in.normal)) * uColor.a, uColor.a) * weight;

	revealValue = uColor.a;
}