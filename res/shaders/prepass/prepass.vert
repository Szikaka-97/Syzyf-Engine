#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;

out vec3 viewNormal;

void main() {
    gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);
    
    vec3 worldNormal = Object_NormalModelMatrix * vNormal;
    viewNormal = mat3(Global_ViewMatrix) * worldNormal;
}
