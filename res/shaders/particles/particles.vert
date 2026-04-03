#version 460 core

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

struct ParticleData {
    vec4 position;
    vec4 velocity;
};

layout(std430, binding = 3) buffer ParticleBuffer {
    ParticleData particles[];
};

out VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} vs_out;

void main() {
    vec4 particle = particles[gl_InstanceID].position;
    vec3 particlePosition = particle.xyz;

	vs_out.worldPos = (mat3(Object_ModelMatrix) * vPos) + particlePosition;

	gl_Position = Global_VPMatrix * vec4(vs_out.worldPos, 1.0);

	vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.normal = Object_NormalModelMatrix * vNormal;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;
}
