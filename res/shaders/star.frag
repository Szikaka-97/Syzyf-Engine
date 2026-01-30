#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

in VARYINGS {
	vec3 normal;
	vec3 worldPos;
	flat uint instanceID;
} ps_in;

out vec4 fragColor;

void main() {
	uint bep = (ps_in.instanceID % 7) + 1;

	fragColor = vec4(
		float(bep & 1),
		float(bep & 2),
		float(bep & 4),
		1.0
	);

	vec3 cameraDirection = normalize(Global_CameraWorldPos - ps_in.worldPos);

	float shadow = clamp(0.0, 1.0, dot(normalize(ps_in.normal), cameraDirection) * 3.6);

	fragColor *= mix(0.15, 4.0, pow(shadow - 1.0, 3.0) + 1.0);
}