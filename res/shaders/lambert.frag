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

#define SHADING_LAMBERT

#include "shared/shading.h"

#include "shared/light.h"

uniform sampler2D colorTex;
uniform vec3 uColor;

out vec4 fragColor;

void main() {
	Material mat;
	mat.diffuseColor = texture(colorTex, ps_in.texcoords).xyz * uColor;
	mat.diffuseStrength = 1.0;

	fragColor = vec4(shade(mat, ps_in.worldPos, normalize(ps_in.normal), vec3(0, 0, 0)), 1.0);
}