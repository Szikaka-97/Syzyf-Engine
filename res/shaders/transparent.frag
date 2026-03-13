#version 460

in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

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
	float weight = calcWeight(uColor.a);

	accumValue = vec4(uColor.rgb * uColor.a, uColor.a) * weight;

	revealValue = uColor.a;
}