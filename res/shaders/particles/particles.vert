#version 460 core

#pragma complex_vertex_shader

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

struct ParticleData {
    vec4 position; // w is size
    vec4 velocity;
    vec4 lifetime;
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
    float alpha;
    float lifetime;
} vs_out;

uniform vec3 areaCenter;

// change later
uniform uint billboardMode;

uniform sampler2D scaleCurveTex;
uniform uint useScaleCurve;

uniform uint distanceFadeMode;
uniform float distanceFadeMin;
uniform float distanceFadeMax;

uniform uint lifetimeFadeMode;
uniform vec2 lifetimeFadeIn;
uniform vec2 lifetimeFadeOut;

uniform uint rotateY;

void main() {
    vec4 particle = particles[gl_InstanceID].position;
    vec4 lifetime = particles[gl_InstanceID].lifetime;

    if (lifetime.x < 0.0) {
        vs_out.alpha = 0.0;
        gl_Position = vec4(2.0, 2.0, 2.0, 0.0);
        return;
    }

    vec3 particlePosition = particle.xyz;

    float c = cos(lifetime.z);
    float s = sin(lifetime.z);
    vec3 rotatedPosition;

    if (rotateY > 0) {
        rotatedPosition = vec3(
            vPos.x * c + vPos.z * s,
            vPos.y,
            -vPos.x * s + vPos.z * c
        );
    } else {
        rotatedPosition = vec3(
            vPos.x * c - vPos.y * s,
            vPos.x * s + vPos.y * c,
            vPos.z
        );
    }

    vs_out.lifetime = clamp(lifetime.x / lifetime.y, 0.0, 1.0);

    float size = particle.w;
    if (useScaleCurve > 0) {
        float scaleMultiplier = texture(scaleCurveTex, vec2(vs_out.lifetime, 0.5)).r;
        size *= scaleMultiplier;
    }

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
        vec3 worldCameraForward = vec3(
            Global_ViewMatrix[0][2],
            Global_ViewMatrix[1][2],
            Global_ViewMatrix[2][2]
        );

        if (billboardMode == 2) {
            worldCameraUp = vec3(0.0, 1.0, 0.0);
            worldCameraForward = normalize(cross(worldCameraRight, worldCameraUp));
        }

        vs_out.worldPos = particlePosition
            + worldCameraRight * rotatedPosition.x * size
            + worldCameraUp * rotatedPosition.y * size
            + worldCameraForward * rotatedPosition.z * size;

        vs_out.normal = cross(worldCameraRight, worldCameraUp);
    } else {
	    vs_out.worldPos = (mat3(Object_ModelMatrix) * rotatedPosition * size) + particlePosition;
        vs_out.normal = Object_NormalModelMatrix * vNormal;
    }

	gl_Position = Global_VPMatrix * vec4(vs_out.worldPos, 1.0);

	vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;

    // Distance fade

    vs_out.alpha = 1.0;

    if (distanceFadeMode > 0) {
        float distanceToCenter = length(particlePosition - areaCenter);
        vs_out.alpha *= 1.0 - smoothstep(distanceFadeMin, distanceFadeMax, distanceToCenter);
    }

    if (lifetimeFadeMode > 0) {
        vs_out.alpha *= smoothstep(lifetimeFadeIn.x, lifetimeFadeIn.y, vs_out.lifetime);
        vs_out.alpha *= 1.0 - smoothstep(lifetimeFadeOut.x, lifetimeFadeOut.y, vs_out.lifetime);
    }
}
