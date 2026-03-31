#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_COLOR) in vec3 vColor;

uniform mat4 u_ViewProjection;

out vec3 pNormal;
out vec3 pColor;

void main() {
	gl_Position = Global_VPMatrix * vec4(vPos.xyz, 1.0);
  pColor = vColor;
}
