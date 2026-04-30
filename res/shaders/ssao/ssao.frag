#version 460 core

layout (location = 1) out float FragColor;

#include "shared/uniforms.h"
#include "shared/shared.h"

in vec2 pUVCoords;

uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;

uniform vec3 samples[64];
uniform vec2 resolution;

uniform int kernelSize;
uniform float radius;
uniform float bias;
uniform float power;

vec3 ViewPosFromDepth(vec2 uv, float rawDepth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, rawDepth * 2.0 - 1.0, 1.0);

    vec4 viewPos = Global_InverseProjectionMatrix * ndc;

    return viewPos.xyz / viewPos.w;
}

void main() {
    float rawDepth = texture(depthTex, pUVCoords).r;
    if (rawDepth >= 1.0) {
        FragColor = 1.0;
        return;
    }

    vec3 fragPos = ViewPosFromDepth(pUVCoords, rawDepth);
    vec3 normal = normalize(texture(normalTex, pUVCoords).rgb);

    vec2 noiseScale = resolution / 4.0;
    vec3 randomVec = normalize(texture(noiseTex, pUVCoords * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * samples[i];
        samplePos = fragPos + samplePos * radius;

        vec4 offset = vec4(samplePos, 1.0);
        offset = Global_ProjectionMatrix * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float rawSampleDepth = texture(depthTex, offset.xy).r;
        float sampleGeometryZ = ViewPosFromDepth(offset.xy, rawSampleDepth).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleGeometryZ));
        occlusion += (sampleGeometryZ >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = pow(occlusion, power);
}
