#ifndef SHADER_LIGHT_H

#ifdef __cplusplus
#error "This file is not supposed to be included in C++ baka"
#else

// #include "shared/shared.h"

#define DIRECTIONAL_LIGHT_CASCADE_COUNT 6

layout (std430, binding = 1) buffer LightInfo {
	vec4 Light_AmbientLight;
	int Light_LightCount;
	Light Light_LightsList[];
};

layout (std430, binding = 0) buffer ShadowmapInfo {
	ShadowMapRegion Light_ShadowMapRegions[];
};

uniform sampler2D shadowMask;

vec3 getLightStrength(in Light light, in vec3 worldPos) {
	if (light.type == DIRECTIONAL_LIGHT) {
		return light.color * light.intensity;
	}

	float dist = distance(light.position, worldPos);

	return light.color * (light.intensity / (1 + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist));
}

vec3 shade(in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
#ifdef SHADING_FUNCTION
#ifndef IGNORE_AMBIENT
	vec3 result = mat.diffuseColor * (Light_AmbientLight.rgb * Light_AmbientLight.a);
#else
	vec3 result = vec3(0, 0, 0);
#endif
	for (int lightIndex = 0; lightIndex < Light_LightCount; lightIndex++) {
		Light l = Light_LightsList[lightIndex];

		if (l.intensity <= 0) {
			continue;
		}

		if (l.type == POINT_LIGHT && distance(worldPos, l.position) > l.range) {
			continue;
		}

		if (l.type == SPOT_LIGHT && (
			distance(worldPos, l.position) > l.range
			||
			dot(normalize(worldPos - l.position), l.direction) < cos(l.spotlightAngle)
		)) {
			continue;
		}

		float shadowAmount = 0.0;

		if (l.shadowAtlasIndex >= 0) {
			vec3 lightDir = normalize(l.position - worldPos);

			float pixelDepth = -ps_in.viewPos.z / Global_CameraFarPlane;

			uint index = 0;

			if (l.type == DIRECTIONAL_LIGHT) {
				index = uint(floor(sqrt(pixelDepth) * DIRECTIONAL_LIGHT_CASCADE_COUNT));

				lightDir = -l.direction;
			}
			else if (l.type == POINT_LIGHT) {
				if (abs(lightDir.x) > abs(lightDir.y) && abs(lightDir.x) > abs(lightDir.z)) {
					index = lightDir.x > 0 ? 1 : 0;
				}
				else if (abs(lightDir.y) > abs(lightDir.z)) {
					index = lightDir.y > 0 ? 3 : 2;
				}
				else {
					index = lightDir.z > 0 ? 5 : 4;
				}
			}

			ShadowMapRegion mask = Light_ShadowMapRegions[l.shadowAtlasIndex + index];

			vec4 lightViewPos = mask.viewTransform * vec4(worldPos, 1);
			lightViewPos /= lightViewPos.w;
			lightViewPos.z = (lightViewPos.z + 1) * 0.5;

			vec2 texelSize = 1.0 / (textureSize(shadowMask, 0) * (mask.end.x - mask.start.x));
			// float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
			float bias = 0;

			vec2 uvLocal = vec2(
				(lightViewPos.x + 1) * 0.5,
				(lightViewPos.y + 1) * 0.5
			);

			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					vec2 uvOffset = clamp(uvLocal + vec2(x, y) * texelSize, 0, 1);

					vec2 uv = mix(mask.start, mask.end, uvOffset);

					float shadowZ = texture(shadowMask, uv).x;

					shadowAmount += lightViewPos.z - bias > shadowZ ? 1.0 : 0.0; 
				}
			}

			shadowAmount /= 9.0;
		}

		result += (1.0 - shadowAmount) * SHADING_FUNCTION(l, mat, worldPos, normal, tangent);
	}

	return result;
#else
	return mat.diffuseColor;
#endif
}

#endif

#define SHADER_LIGHT_H
#endif