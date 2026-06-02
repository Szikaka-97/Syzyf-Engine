#version 460

#pragma additive

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

#define SHADING_LAMBERT

#include "shared/shading.h"
#include "shared/light.h"

uniform sampler2D colorTex;
uniform vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
uniform float colorIntensity = 1.0f;
uniform sampler2D colorRamp;
uniform sampler2D depthTex;
uniform float depthFadeDistance;
uniform uint enableDepthFade;
uniform uint useColorRamp;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;

out vec4 FragColor;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) / (Global_CameraFarPlane + Global_CameraNearPlane - z * (Global_CameraFarPlane - Global_CameraNearPlane));
}

void main() {
    vec4 baseColor = texture(colorTex, ps_in.texcoords) * color;
    
    if (useColorRamp > 0) {
        vec4 rampColor = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        baseColor.rgb *= rampColor.rgb * colorIntensity;
    }

    float finalAlpha = baseColor.a * ps_in.alpha;

    // Doesn't work well with large quads in the vertex shader
    //  if the performance is bad for smaller particles try moving this there again
    if (proximityFadeMode > 0) {
        float distanceToCamera = length(ps_in.viewPos);
        finalAlpha *= smoothstep(proximityFadeMin, proximityFadeMax, distanceToCamera);
    }

    if (enableDepthFade > 0) {
        vec2 screenSize = vec2(textureSize(depthTex, 0));

        vec2 screenUV = gl_FragCoord.xy / screenSize;
        float rawDepth = texture(depthTex, screenUV).r;
        float sceneDepth = LinearizeDepth(rawDepth);
        float particleDepth = LinearizeDepth(gl_FragCoord.z);

        float distanceToScene = sceneDepth - particleDepth;

        float depthFade = clamp(distanceToScene / depthFadeDistance, 0.0, 1.0);
        finalAlpha *= depthFade;
    }

    FragColor = vec4(baseColor.rgb * finalAlpha, finalAlpha);
}
