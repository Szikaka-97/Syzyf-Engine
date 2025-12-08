#version 460

#include "shared/shared.h"

struct Material {
	vec3 diffuseColor;
	vec3 specularColor;
	float diffuseStrength;
	float specularStrength;
	float specularHighlight;
};

#define SHADING_FUNCTION shadeLambert
vec3 shadeLambert(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent);

#include "shared/light.h"

in VS_OUT {
	vec3 worldPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

vec3 shadeLambert(in Light light, in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
	vec3 direction = light.type == POINT_LIGHT ? normalize(worldPos - light.position) : light.direction;

	return getLightStrength(light, worldPos) * mat.diffuseColor * dot(direction, -normal);
}

uniform sampler2D colorTex;
uniform vec3 uColor;

out vec4 fragColor;

void main() {
	Material mat;
	mat.diffuseColor = texture(colorTex, ps_in.texcoords).xyz * uColor;

	fragColor = vec4(shade(mat, ps_in.worldPos, normalize(ps_in.normal), vec3(0, 0, 0)), 1.0);
}