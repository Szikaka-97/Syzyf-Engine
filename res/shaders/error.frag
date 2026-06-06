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

uniform vec3 uColor;
uniform float specularValue;

out vec4 fragColor;

void main() {
	vec3 N = normalize(ps_in.normal);
	vec3 V = normalize(Global_PlayerWorldPos.xyz - ps_in.worldPos);
	
	vec3 color = vec3(1.2, 0.2, 0.4);

	float diff = max(dot(N, V), 0.0);

	fragColor = vec4(color * diff, 1.0);
}