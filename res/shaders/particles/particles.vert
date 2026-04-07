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
} vs_out;

uniform vec3 areaCenter;

// change later
uniform uint billboardMode;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;

uniform uint distanceFadeMode;
uniform float distanceFadeMin;
uniform float distanceFadeMax;

void main() {
    vec4 particle = particles[gl_InstanceID].position;
    vec4 lifetime = particles[gl_InstanceID].lifetime;
    vec3 particlePosition = particle.xyz;

    float c = cos(lifetime.z);
    float s = sin(lifetime.z);
    mat2 rotation = mat2(c, -s, s, c);
    vec2 rotatedPosition = rotation * vPos.xy;

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
            + worldCameraRight * rotatedPosition.x * particle.w
            + worldCameraUp * rotatedPosition.y * particle.w;

        vs_out.normal = cross(worldCameraRight, worldCameraUp);
    } else {
	    vs_out.worldPos = (mat3(Object_ModelMatrix) * (vec3(rotatedPosition, vPos.z) * particle.w)) + particlePosition;
        vs_out.normal = Object_NormalModelMatrix * vNormal;
    }

	gl_Position = Global_VPMatrix * vec4(vs_out.worldPos, 1.0);

	vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;

    // Distance fade
    // Doesn't always work with large quads, could move to the fragment shader if necessary
    vs_out.alpha = 1.0;
    float distanceToCenter = length((Global_ViewMatrix * vec4(particle.xyz, 1.0)).xyz);
    float distanceToEdge = distanceToCenter - (particle.w * 0.5);

    if (proximityFadeMode > 0) {
        vs_out.alpha *= smoothstep(proximityFadeMin, proximityFadeMax, distanceToEdge);
    }
    // remove distance fade, replace with lifetime
    if (distanceFadeMode > 0) {
        vs_out.alpha *= 1.0 - smoothstep(distanceFadeMin, distanceFadeMax, lifetime.x);
        // fade in, chagne
        vs_out.alpha *= smoothstep(0.0, distanceFadeMax - distanceFadeMin, lifetime.x);
    }
}
