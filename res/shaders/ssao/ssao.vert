#version 460 core

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_UV1) in vec2 vUVCoords;

out vec2 pUVCoords;

void main() {
    gl_Position = vec4(vPos.xyz, 1.0);
    pUVCoords = vUVCoords;
}
