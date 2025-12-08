#ifndef SHADER_LIGHT_H

#ifdef __cplusplus
#error "This file is not supposed to be included in C++ baka"
#endif

layout (std430, binding = 1) buffer LightInfo {
	vec4 Light_AmbientLight;
	int Light_LightCount;
	Light Light_LightsList[];
};

vec3 getLightStrength(in Light light, in vec3 worldPos) {
	if (light.type == DIRECTIONAL_LIGHT) {
		return light.color;
	}

	return light.color * (light.intensity / distance(light.position, worldPos));
}

vec3 shade(in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
#ifdef SHADING_FUNCTION
	vec3 result = mat.diffuseColor * (Light_AmbientLight.rgb * Light_AmbientLight.a);

	for (int lightIndex = 0; lightIndex < Light_LightCount; lightIndex++) {
		Light l = Light_LightsList[lightIndex];

		if (l.type == POINT_LIGHT && distance(worldPos, l.position) > l.range) {
			continue;
		}

		if (l.type == SPOT_LIGHT && (
			distance(worldPos, l.position) > l.range
			||
			dot(normalize(worldPos - l.position), l.direction) > cos(l.spotlightAngle)
		)) {
			continue;
		}

		result += SHADING_FUNCTION(l, mat, worldPos, normal, tangent);
	}

	return result;
#else
	return mat.diffuseColor;
#endif
}

#define SHADER_LIGHT_H
#endif