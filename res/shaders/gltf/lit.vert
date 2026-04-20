#version 460

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

void main() {
	gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);

	vs_out.worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
	vs_out.viewPos = (Global_ViewMatrix * (Object_ModelMatrix * vec4(vPos, 1.0))).xyz;
	vs_out.normal = Object_NormalModelMatrix * vNormal;
	vs_out.tangent.xyz = Object_NormalModelMatrix * vTangent.xyz;
  vs_out.tangent.w = vTangent.w;
	vs_out.texcoords = vUVCoords;
  vs_out.texcoords2 = vUVCoords2;
}
