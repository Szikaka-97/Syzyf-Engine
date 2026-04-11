#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout(IN_POSITION) in vec3 vPos;

out VS_OUT {
    vec3 worldPos;
} vs_out;

void main() {
    gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);

    vs_out.worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
}
