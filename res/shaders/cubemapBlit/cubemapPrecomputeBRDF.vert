#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_UV1) in vec2 vUVCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = vUVCoords;
	gl_Position = vec4(vPos, 1.0);
}