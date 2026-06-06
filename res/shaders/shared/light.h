#ifndef SHADER_LIGHT_H

#ifdef __cplusplus
#error "This file is not supposed to be included in C++ baka"
#else

// #include "shared/shared.h"

layout (std430, binding = 0) restrict buffer ShadowmapInfo {
	ShadowMapRegion Light_ShadowMapRegions[];
};

layout (std430, binding = 1) restrict buffer LightInfo {
	vec4 Light_AmbientLight;
	int Light_LightCount;
	int Light_DirectionalLightCascadeCount;
	Light Light_LightsList[];
};

layout (std430, binding = 2) restrict buffer LightIndexList {
	uvec4 Light_LightGridSize;
	uint Light_LightIndexList[];
};

layout (std430, binding = 5) restrict buffer LightGrid {
	uvec2 Light_LightGrid[];
};

void Light_AddLight(Light l) {
	if (Light_LightCount >= Light_LightsList.length()) {
		return;
	}
	
	uint lightIndex = atomicAdd(Light_LightCount, 1);

	Light_LightsList[lightIndex] = l;
}

uniform sampler2D Builtin_ShadowMask;

#ifdef OLD_LIGHT_FALLOFF
vec3 getLightStrength(in Light light, in vec3 worldPos) {
	if (light.type == DIRECTIONAL_LIGHT) {
		return light.color * light.intensity;
	}

	float dist = distance(light.position, worldPos);

	return light.color * (light.intensity / (1 + light.linearAttenuation * dist + light.quadraticAttenuation * dist * dist));
}
#else
// Frostbite Falloff
vec3 getLightStrength(in Light light, in vec3 worldPos) {
	if (light.type == DIRECTIONAL_LIGHT) {
		return light.color * light.intensity;
	}

	float dist = distance(light.position, worldPos);

	float denominator = max(dist, 0.01);
	denominator *= denominator;

	// Windowing function
	float distanceOverRadius = dist / light.range;
	float distanceOverRadius4 = distanceOverRadius * distanceOverRadius * distanceOverRadius * distanceOverRadius;

	float window = clamp(1.0 - distanceOverRadius4, 0.0, 1.0);
	window *= window;

	return (light.color * (light.intensity / denominator)) * window;
}
#endif

#ifdef SHADING_FUNCTION
vec3 shade(in Material mat, in vec3 worldPos, in vec3 normal, in vec3 tangent) {
#ifndef IGNORE_AMBIENT
	vec3 result = mat.diffuseColor * (Light_AmbientLight.rgb * Light_AmbientLight.a);
#else
	vec3 result = vec3(0, 0, 0);
#endif
	const vec3 viewPos = (Global_ViewMatrix * vec4(worldPos, 1.0)).xyz;

	const uint zTile = uint((log(abs(viewPos.z) / Global_CameraNearPlane) * Light_LightGridSize.z) / log(Global_CameraFarPlane / Global_CameraNearPlane));
	const vec2 tileSize = Global_Resolution.xy / Light_LightGridSize.xy;

	const ivec3 tile = ivec3(gl_FragCoord.xy / tileSize, zTile);

	const uint clusterIndex = uint(dot(tile, vec3(1, Light_LightGridSize.x, Light_LightGridSize.x * Light_LightGridSize.y)));

	const uvec2 lightData = Light_LightGrid[clusterIndex];
	
	const uint lightStartIndex = lightData.x;
	const uint lightCount = lightData.y;

	for (int lightIndex = 0; lightIndex < lightCount; lightIndex++) {
		Light l = Light_LightsList[Light_LightIndexList[lightStartIndex + lightIndex]];
	// for (int lightIndex = 0; lightIndex < 8192; lightIndex++) {
	// 	Light l = Light_LightsList[lightIndex];

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
				index = uint(floor(sqrt(pixelDepth) * Light_DirectionalLightCascadeCount));

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

			vec2 texelSize = 1.0 / (textureSize(Builtin_ShadowMask, 0) * (mask.end.x - mask.start.x));
			float bias = max(0.002 * (1.0 - dot(normal, lightDir)), 0.001);
			if (l.type == DIRECTIONAL_LIGHT) {
				bias *= (index + 1) * 0.5;
			}
			// float bias = 0;

			vec2 uvLocal = clamp(vec2(
				(lightViewPos.x + 1) * 0.5,
				(lightViewPos.y + 1) * 0.5
			), 0, 1);

			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					vec2 uvOffset = clamp(uvLocal + vec2(x, y) * texelSize, 0, 1);

					vec2 uv = mix(mask.start, mask.end, uvOffset);

					float shadowZ = texture(Builtin_ShadowMask, uv).x;

					shadowAmount += lightViewPos.z - bias > shadowZ ? 1.0 : 0.0; 
				}
			}

			shadowAmount /= 9.0;
		}

		result += (1.0 - shadowAmount) * SHADING_FUNCTION(l, mat, worldPos, normal, tangent);
	}

	return result;
}
#endif

#endif

#define SHADER_LIGHT_H
#endif
