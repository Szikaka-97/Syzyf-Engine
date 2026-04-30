#version 460
#pragma complex_vertex_shader
#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_JOINTS) in vec4 vJoints;
layout (IN_WEIGHTS) in vec4 vWeights;

out vec3 viewNormal;

layout(std430, binding = 2) readonly buffer SkinningBuffer {
    mat4 jointMatrices[];
};
uniform int uBoneOffset;

const int MAX_BONE_INFLUENCE = 4;

void main() {
    vec4 totalPosition = vec4(0.0f);
    vec3 totalNormal = vec3(0.0f);

    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        int joint = int(vJoints[i]);
        float weight = vWeights[i];
        if (weight <= 0.0f || joint < 0) continue;

        mat4 jointMat = jointMatrices[uBoneOffset + joint];
        totalPosition += (jointMat * vec4(vPos, 1.0f)) * weight;
        totalNormal += (mat3(jointMat) * vNormal) * weight;
    }

    if (totalPosition == vec4(0.0f)) {
        totalPosition = vec4(vPos, 1.0f);
        totalNormal = vNormal;
    }

    gl_Position = Object_MVPMatrix * totalPosition;
    
    vec3 worldNormal = Object_NormalModelMatrix * totalNormal;
    viewNormal = mat3(Global_ViewMatrix) * worldNormal;
}
