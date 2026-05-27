#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

in vec3 viewNormal;
in vec2 pTexCoords;
in vec3 worldPos;

layout (location = 1) out vec3 outNormal;

layout(location = 0) uniform vec4 baseColorFactor;
layout(location = 1) uniform float alphaCutoff;
layout(location = 2) uniform sampler2D albedoMap;
layout(location = 3) uniform sampler2D ditherTex;

layout(location = 20) uniform sampler2D Builtin_ShadowMask;

layout(location = 21) uniform float roughnessFactor;
layout(location = 22) uniform float metallicFactor;
layout(location = 23) uniform sampler2D armMap;
layout(location = 24) uniform bool useOcclusion;
layout(location = 25) uniform sampler2D normalMap;
layout(location = 26) uniform vec3 emissiveFactor;
layout(location = 27) uniform float emissiveStrength;
layout(location = 28) uniform sampler2D emissiveMap;

layout(location = 29) uniform samplerCube Builtin_EnvIrradianceMap;
layout(location = 30) uniform samplerCube Builtin_EnvPrefilterMap;
layout(location = 31) uniform sampler2D Builtin_BRDFConvolutionMap;
layout(location = 32) uniform sampler2D Builtin_AOMap;

void main() {
    if (Global_PlayerWorldPos.w > 0.5) {
        vec3 camToPlayer = Global_PlayerWorldPos.xyz - Global_CameraWorldPos.xyz;
        float distToPlayer = length(camToPlayer);
        vec3 dirToPlayer = camToPlayer / distToPlayer;

        vec3 camToFrag = worldPos - Global_CameraWorldPos.xyz;
        float t = dot(camToFrag, dirToPlayer);

        if (t > 0.0 && t < distToPlayer) {
            float distToLine = length(camToFrag - dirToPlayer * t);
            
            float holeRadius = 1.5; 
            float fadeWidth = 1.0;

            if (distToLine < holeRadius + fadeWidth) {
                float alpha = smoothstep(holeRadius, holeRadius + fadeWidth, distToLine);
                
                ivec2 texSize = textureSize(ditherTex, 0);
                ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % texSize;
                float ditherVal = texelFetch(ditherTex, ditherCoord, 0).r;

                if (alpha < ditherVal) {
                    discard;
                }
            }
        }
    }

    float baseAlpha = baseColorFactor.a == 0.0 ? 1.0 : baseColorFactor.a;
    float texAlpha = texture(albedoMap, pTexCoords).a * baseAlpha;

    if (texAlpha < alphaCutoff) {
        discard;
    }

    outNormal = normalize(viewNormal);
}
