#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_UV1) in vec2 vUVCoords;

out vec3 viewNormal;
out vec2 pTexCoords;
out vec3 worldPos;

void main() {
    gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);
    vec3 worldNormal = Object_NormalModelMatrix * vNormal;
    viewNormal = mat3(Global_ViewMatrix) * worldNormal;
    pTexCoords = vUVCoords;
    worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
}
