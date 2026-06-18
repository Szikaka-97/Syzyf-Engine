#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec4 vTangent;
layout (IN_UV1) in vec2 vUVCoords;
layout (IN_UV2) in vec2 vUVCoords2;

out VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec4 tangent;
	vec2 texcoords;
	vec2 texcoords2;
} vs_out;
