#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

in VARYINGS {
	vec3 normal;
	vec3 worldPos;
} ps_in;

out vec4 fragColor;

void main() {
	fragColor = vec4(1.0, 1.0, 1.0, 1.0);

	vec3 cameraDirection = normalize(Global_CameraWorldPos - ps_in.worldPos);

	float shadow = clamp(0.0, 1.0, dot(normalize(ps_in.normal), cameraDirection) * 3.6);

	fragColor *= mix(0.15, 10.0, pow(shadow - 1.0, 3.0) + 1.0);

	gl_FragDepth = 0.9999;
}