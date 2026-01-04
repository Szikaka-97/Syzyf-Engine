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

#define SHADING_PHONG

#include "shared/shading.h"

#include "shared/light.h"

uniform vec3 uColor;
uniform float specularValue;

out vec4 fragColor;

void main() {
	Material mat = defaultMaterial();
	mat.diffuseColor = uColor;
	mat.specularColor = vec3(1, 1, 1);
	mat.specularHighlight = specularValue;

	fragColor = vec4(shade(mat, ps_in.worldPos, normalize(ps_in.normal), vec3(0, 0, 0)), 1.0);
}