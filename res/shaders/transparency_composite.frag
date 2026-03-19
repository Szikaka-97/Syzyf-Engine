#version 460

layout (binding = 0) uniform sampler2D accum;
layout (binding = 1) uniform sampler2D reveal;

out vec4 fragColor;

const float EPSILON = 0.00001f;

bool approximatelyEqual(float a, float b) {
	return abs(a - b) <= EPSILON;
}

// get the max value between three values
float max3(vec3 v) {
	return max(v.x, max(v.y, v.z));
}

void main() {
	const ivec2 pixelCoord = ivec2(gl_FragCoord.xy);

	const float revealage = texelFetch(reveal, pixelCoord, 0).r;

	if (approximatelyEqual(revealage, 1.0f)) {
		discard;
	}

	vec4 accumulation = texelFetch(accum, pixelCoord, 0);

	if (isinf(max3(abs(accumulation.rgb)))) {
		accumulation.rgb = vec3(accumulation.a);
	}

	const vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

	fragColor = vec4(average_color, 1.0f - revealage);
}