#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec4 vTangent;
layout (IN_UV1) in vec2 vUVCoords;
layout (IN_UV2) in vec2 vUVCoords2;
layout (IN_JOINTS) in vec4 vJoints;
layout (IN_WEIGHTS) in vec4 vWeights;

out VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec4 tangent;
	vec2 texcoords;
  vec2 texcoords2;
} vs_out;

layout(std430, binding = 2) readonly buffer SkinningBuffer {
  mat4 jointMatrices[];
};

uniform int uBoneOffset;

//from learnopengl
const int MAX_BONE_INFLUENCE = 4;

void main() {
  vec4 totalPosition = vec4(0.0f);
  vec3 totalNormal = vec3(0.0f);
  vec3 totalTangent = vec3(0.0f);

  for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
    int joint = int(vJoints[i]);
    float weight = vWeights[i];

    if (weight <= 0.0f || joint < 0) {
      continue;
    }

    mat4 jointMat = jointMatrices[uBoneOffset + joint];
        
    vec4 localPosition = jointMat * vec4(vPos, 1.0f);
    totalPosition += localPosition * weight;

    vec3 localNormal = mat3(jointMat) * vNormal;
    totalNormal += localNormal * weight;
        
    vec3 localTangent = mat3(jointMat) * vTangent.xyz;
    totalTangent += localTangent * weight;
  }

  if (totalPosition == vec4(0.0f)) {
    totalPosition = vec4(vPos, 1.0f);
    totalNormal = vNormal;
    totalTangent = vTangent.xyz;
  }

  gl_Position = Object_MVPMatrix * totalPosition;

  vs_out.worldPos = (Object_ModelMatrix * totalPosition).xyz;
  vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
    
  vs_out.normal = normalize(Object_NormalModelMatrix * totalNormal);
  vs_out.tangent.xyz = normalize(Object_NormalModelMatrix * totalTangent);
  vs_out.tangent.w = vTangent.w;
    
  vs_out.texcoords = vUVCoords;
  vs_out.texcoords2 = vUVCoords2;
}
