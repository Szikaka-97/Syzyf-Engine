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

#pragma no_depth_prepass
#pragma no_shadows

uniform sampler2D colorTex;

out vec4 fragColor;

void main() {
	vec4 col = texture(colorTex, ps_in.texcoords);

	if (col.a < 0.5) {
		discard;
	}

	fragColor = col;
}