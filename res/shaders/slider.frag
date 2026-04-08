#version 460

in vec2 texcoords;

#include "shared/shared.h"
#include "shared/uniforms.h"

#pragma no_depth_prepass
#pragma no_shadows

uniform float amount;

out vec4 fragColor;

void main() {
	if (texcoords.x > amount) {
		discard;
	}

	fragColor = vec4(mix(vec3(1, 0, 0), vec3(0, 1, 0), amount), 1);
}