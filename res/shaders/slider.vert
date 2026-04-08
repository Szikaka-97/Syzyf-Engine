#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_UV1) in vec2 vUVCoords;

out vec2 texcoords;

void main() {
	gl_Position = Object_MVPMatrix * vec4(vPos.xyz, 1.0);
	gl_Position.z = -gl_Position.w;

	texcoords = vUVCoords;
}