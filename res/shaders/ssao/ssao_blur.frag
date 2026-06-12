#version 460 core

layout (location = 1) out float FragAO;
layout (location = 2) out vec3 FragSSGI;

#include "shared/uniforms.h"
#include "shared/shared.h"

in vec2 pUVCoords;

uniform sampler2D ssaoTex;
uniform sampler2D ssgiTex;
uniform sampler2D depthTex;
uniform sampler2D normalTex;

uniform int blurRange;

float LinearizeDepth(float rawDepth) {
    float zNear = Global_CameraNearPlane;
    float zFar  = Global_CameraFarPlane;
    float ndc = rawDepth * 2.0 - 1.0;
    return (2.0 * zNear * zFar) / (zFar + zNear - ndc * (zFar - zNear));
}

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTex, 0));

    float resultAO = 0.0;
    vec3 resultSSGI = vec3(0.0);
    float weightSum = 0.0;

    float centerDepth = LinearizeDepth(texture(depthTex, pUVCoords).r);
    vec3 centerNormal = texture(normalTex, pUVCoords).rgb;

    float depthThreshold = 0.2;
    float normalThreshold = 8.0;

    for (int x = -blurRange; x < blurRange; ++x) {
        for (int y = -blurRange; y < blurRange; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            vec2 sampleUV = pUVCoords + offset;

            float sampleAO = texture(ssaoTex, sampleUV).r;
            vec3 sampleSSGI = texture(ssgiTex, sampleUV).rgb;
            float sampleDepth = LinearizeDepth(texture(depthTex, sampleUV).r);
            vec3 sampleNormal = texture(normalTex, sampleUV).rgb;

            float spatialWeight = exp(-0.5 * float(x * x + y * y) / float(blurRange * blurRange));

            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = exp(-depthDiff * depthDiff / (2.0 * depthThreshold * depthThreshold));

            float normalDot = max(dot(centerNormal, sampleNormal), 0.0);
            float normalWeight = pow(normalDot, normalThreshold);

            float totalWeight = spatialWeight * depthWeight * normalWeight;

            resultAO += sampleAO * totalWeight;
            resultSSGI += sampleSSGI * totalWeight;
            weightSum += totalWeight;

        }
    }

    FragAO = resultAO / weightSum;
    FragSSGI = resultSSGI / weightSum;
}
