#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;

void main() {
	gl_Position = Object_MVPMatrix * vec4(vPos.xyz, 1.0);
}