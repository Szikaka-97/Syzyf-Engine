#ifndef SHADER_SHARED_H

#ifdef __cplusplus

#pragma once

#include <glm/glm.hpp>
#define vec3 glm::vec3
#define vec2 glm::vec2
#define t_bool unsigned int
#define mat4 glm::mat4

#else

#define t_bool uint

#endif

#ifndef __cplusplus

#define IN_POSITION location=0
#define IN_NORMAL location=1
#define IN_BINORMAL location=2
#define IN_TANGENT location=3
#define IN_UV1 location=4
#define IN_UV2 location=5
#define IN_COLOR location=6

#define POINT_LIGHT 0
#define SPOT_LIGHT 1
#define DIRECTIONAL_LIGHT 2

float inverseLerp(in float a, in float b, in float v) {
	return (v - a) / (b - a);
}

#endif

#ifdef __cplusplus
struct ShaderLightRep {
#else
struct Light {
#endif
	vec3 position;
	uint type;
	vec3 direction;
	float range;
	vec3 color;
	float spotlightAngle;
	float intensity;
	float linearAttenuation;
	float quadraticAttenuation;
	int shadowAtlasIndex;
};

struct ShadowMapRegion {
	mat4 viewTransform;
	vec2 start;
	vec2 end;
};

#ifdef vec3
#undef vec3
#endif

#ifdef vec2
#undef vec2
#endif

#ifdef mat4
#undef mat4
#endif

#undef t_bool

#define SHADER_SHARED_H
#endif