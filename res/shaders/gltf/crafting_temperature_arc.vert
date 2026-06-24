#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_UV1) in vec2 vUVCoords;

out VS_OUT {
    vec3 localPos;
    vec2 texcoords;
} vs_out;

void main(){
    gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);

    vs_out.localPos = vPos;
    vs_out.texcoords = vUVCoords;
}
