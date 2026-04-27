#version 460 core

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

struct InstanceData {
    mat4 transform;
};

layout(std430, binding = 3) readonly buffer ScatterInstanceBuffer {
InstanceData instances[];
};

out VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
} vs_out;

void main() {
    mat4 instanceTransform = instances[gl_InstanceID].transform;
    mat4 finalModelMatrix = Object_ModelMatrix * instanceTransform;

    gl_Position = Global_VPMatrix * finalModelMatrix * vec4(vPos, 1.0);
    vs_out.worldPos = (finalModelMatrix * vec4(vPos, 1.0)).xyz;
    vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
    mat3 normalMatrix = mat3(finalModelMatrix);
    vs_out.normal = normalize(normalMatrix * vNormal);
    vs_out.tangent = normalize(normalMatrix * vTangent);

    vs_out.texcoords = vUVCoords;
}
