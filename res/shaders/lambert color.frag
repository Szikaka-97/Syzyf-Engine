#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

#include "shared/shading.h"

#define SHADING_FUNCTION shadePhong

#include "shared/light.h"

in VS_OUT {
	vec3 worldPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

uniform vec3 uColor;

out vec4 fragColor;

void main() {
	Material mat;
	mat.diffuseColor = uColor;
	mat.specularColor = vec3(1, 1, 1);
	mat.specularHighlight = 250;

	fragColor = vec4(shade(mat, ps_in.worldPos, normalize(ps_in.normal), vec3(0, 0, 0)), 1.0);
	// fragColor = vec4(ps_in.worldPos, 1.0);
}