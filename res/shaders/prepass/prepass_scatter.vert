#version 460 core
#pragma complex_vertex_shader
#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;

struct InstanceData {
    mat4 transform;
};
layout(std430, binding = 3) readonly buffer ScatterInstanceBuffer { 
    InstanceData instances[]; 
};

out vec3 viewNormal;

void main() {
    mat4 instanceTransform = instances[gl_InstanceID].transform;
    mat4 finalModelMatrix = Object_ModelMatrix * instanceTransform;
    
    gl_Position = Global_VPMatrix * finalModelMatrix * vec4(vPos, 1.0);
    
    mat3 normalMatrix = mat3(finalModelMatrix);
    vec3 worldNormal = normalize(normalMatrix * vNormal);
    viewNormal = mat3(Global_ViewMatrix) * worldNormal;
}
