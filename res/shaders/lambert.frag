#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

#include "shared/shading.h"

#define SHADING_FUNCTION shadeLambert

#include "shared/light.h"

in VS_OUT {
	vec3 worldPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

uniform sampler2D colorTex;
uniform vec3 uColor;

out vec4 fragColor;

void main() {
	Material mat;
	mat.diffuseColor = texture(colorTex, ps_in.texcoords).xyz * uColor;

	fragColor = vec4(shade(mat, ps_in.worldPos, normalize(ps_in.normal), vec3(0, 0, 0)), 1.0);
}