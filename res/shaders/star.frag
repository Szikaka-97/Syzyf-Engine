#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

in vec3 pNormal;
in vec3 pWorldPos;

out vec4 fragColor;

void main() {
	fragColor = vec4(1.0, 1.0, 1.0, 1.0);

	vec3 cameraDirection = normalize(Global_CameraWorldPos - pWorldPos);

	float shadow = clamp(0.0, 1.0, dot(normalize(pNormal), cameraDirection) * 3.6);

	fragColor *= mix(0.15, 1.01, pow(shadow - 1.0, 3.0) + 1.0);

	gl_FragDepth = 0.9999;
}