#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

out VS_OUT {
  vec4 clipPos;
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} vs_out;

void main() {
  vs_out.clipPos = Object_MVPMatrix * vec4(vPos, 1.0);
	gl_Position = vs_out.clipPos;

	vs_out.worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
	vs_out.viewPos = (Global_ViewMatrix * (Object_ModelMatrix * vec4(vPos, 1.0))).xyz;
	vs_out.normal = Object_NormalModelMatrix * vNormal;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;
}
