#version 460 core

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

struct ParticleData {
    vec4 position; // w is size
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

// change later
uniform uint billboardMode;

void main() {
    vec4 particle = particles[gl_InstanceID].position;
    vec3 particlePosition = particle.xyz;

    if (billboardMode > 0) {
        vec3 worldCameraRight = vec3(
            Global_ViewMatrix[0][0],
            Global_ViewMatrix[1][0],
            Global_ViewMatrix[2][0]
        );
        vec3 worldCameraUp = vec3(
            Global_ViewMatrix[0][1],
            Global_ViewMatrix[1][1],
            Global_ViewMatrix[2][1]
        );

        if (billboardMode == 2) {
            worldCameraUp = vec3(0.0, 1.0, 0.0);
        }

        vs_out.worldPos = particlePosition
            + worldCameraRight * vPos.x * particle.w
            + worldCameraUp * vPos.y * particle.w;

        vs_out.normal = cross(worldCameraRight, worldCameraUp);
    } else {
	    vs_out.worldPos = (mat3(Object_ModelMatrix) * (vPos * particle.w)) + particlePosition;
        vs_out.normal = Object_NormalModelMatrix * vNormal;
    }

	gl_Position = Global_VPMatrix * vec4(vs_out.worldPos, 1.0);

	vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;
}
