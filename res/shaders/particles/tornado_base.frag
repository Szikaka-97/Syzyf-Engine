#version 460

in VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
    float alpha;
    float lifetime;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

uniform sampler2D colorTex;
uniform sampler2D colorRamp;
uniform sampler2D depthTex;
uniform float depthFadeDistance;
uniform uint enableDepthFade;
uniform uint useColorRamp;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;

layout (location = 0) out vec4 accum;
layout (location = 1) out float reveal;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) / (Global_CameraFarPlane + Global_CameraNearPlane - z * (Global_CameraFarPlane - Global_CameraNearPlane));
}

void main() {
    vec4 color = texture(colorTex, ps_in.texcoords);

    if (color.r * 5.0 < ps_in.lifetime) {
        discard;
    }

    if (useColorRamp > 0) {
        vec4 colorRamp = texture(colorRamp, vec2(clamp(1.0 - ps_in.lifetime, 0.2, 0.8), 0.0));
        color.rgb *= colorRamp.rgb * 5.0;
    }

    float alpha = color.a * ps_in.alpha * 0.8;

    if (proximityFadeMode > 0) {
        float distanceToCamera = length(ps_in.viewPos);
        alpha *= smoothstep(proximityFadeMin, proximityFadeMax, distanceToCamera);
    }

    if (enableDepthFade > 0) {
        vec2 screenSize = vec2(textureSize(depthTex, 0));

        vec2 screenUV = gl_FragCoord.xy / screenSize;
        float rawDepth = texture(depthTex, screenUV).r;
        float sceneDepth = LinearizeDepth(rawDepth);
        float particleDepth = LinearizeDepth(gl_FragCoord.z);

        float distanceToScene = sceneDepth - particleDepth;

        float depthFade = clamp(distanceToScene / depthFadeDistance, 0.0, 1.0);
        alpha *= depthFade;
    }

    if (alpha < 0.001) discard;

    float weight = clamp(pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
    
    accum = vec4(color.rgb * alpha, alpha) * weight;
    reveal = alpha;
}
