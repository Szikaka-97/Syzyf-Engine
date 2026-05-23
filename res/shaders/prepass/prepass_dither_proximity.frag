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

void main() {
    float distToCamera = distance(Global_CameraWorldPos.xyz, worldPos);
    float fadeThreshold = smoothstep(1.0, 3.0, distToCamera);

    ivec2 texSize = textureSize(ditherTex, 0);
    ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % texSize;
    float ditherVal = texelFetch(ditherTex, ditherCoord, 0).r;
    
    if (fadeThreshold < ditherVal) {
        discard;
    }

    float baseAlpha = baseColorFactor.a == 0.0 ? 1.0 : baseColorFactor.a;
    float texAlpha = texture(albedoMap, pTexCoords).a * baseAlpha;

    if (texAlpha < alphaCutoff) {
        discard;
    }

    outNormal = normalize(viewNormal);
}
