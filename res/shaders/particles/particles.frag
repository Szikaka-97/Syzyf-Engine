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

#define SHADING_LAMBERT

#include "shared/shading.h"

#include "shared/light.h"

uniform sampler2D colorTex;
uniform sampler2D colorRamp;
uniform sampler2D depthTex;
uniform float depthFadeDistance;
uniform uint enableDepthFade;
uniform uint useColorRamp;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;

layout (location = 0) out vec4 accumValue;
layout (location = 1) out float revealValue;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) / (Global_CameraFarPlane + Global_CameraNearPlane - z * (Global_CameraFarPlane - Global_CameraNearPlane));
}

float calcWeight(float alpha) {
	return clamp(
		pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
		0.001,
		3000.0
	);
}

void main() {
	vec4 color = texture(colorTex, ps_in.texcoords);
    if (useColorRamp > 0) {
        vec4 colorRamp = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        color.rgb *= colorRamp.rgb * 5.0;
    }

    float alpha = color.a * ps_in.alpha;

    // Doesn't work well with large quads in the vertex shader
    //  if the performance is bad for smaller particles try moving this there again
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
	
    const float weight = calcWeight(alpha);
    const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);
    
    accumValue = vec4(color.rgb * dot(viewDir, normalize(ps_in.normal)) * alpha, alpha) * weight;
    revealValue = alpha;
}
