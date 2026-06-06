#version 460

#pragma transparent

in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
    float alpha;
    float lifetime;
    flat uint index;
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

uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}

float random(inout uint state) {
    state = hash(state);
    return float(hash(state)) / 4294967295.0;
}

float hue2rgb(float f1, float f2, float hue) {
    if (hue < 0.0)
        hue += 1.0;
    else if (hue > 1.0)
        hue -= 1.0;
    float res;
    if ((6.0 * hue) < 1.0)
        res = f1 + (f2 - f1) * 6.0 * hue;
    else if ((2.0 * hue) < 1.0)
        res = f2;
    else if ((3.0 * hue) < 2.0)
        res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
    else
        res = f1;
    return res;
}

vec3 hsl2rgb(vec3 hsl) {
    vec3 rgb;
    
    if (hsl.y == 0.0) {
        rgb = vec3(hsl.z); // Luminance
    } else {
        float f2;
        
        if (hsl.z < 0.5)
            f2 = hsl.z * (1.0 + hsl.y);
        else
            f2 = hsl.z + hsl.y - hsl.y * hsl.z;
            
        float f1 = 2.0 * hsl.z - f2;
        
        rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));
        rgb.g = hue2rgb(f1, f2, hsl.x);
        rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));
    }   
    return rgb;
}

vec3 hsl2rgb(float h, float s, float l) {
    return hsl2rgb(vec3(h, s, l));
}

void main() {
	vec4 color = texture(colorTex, ps_in.texcoords) * color;
    if (useColorRamp > 0) {
        vec4 colorRamp = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        color.rgb *= colorRamp.rgb * colorIntensity;
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

    uint randomState = uint(ps_in.index);

    FragColor = vec4(color.rgb * hsl2rgb(random(randomState), 1.0f, 0.5f), alpha);
}
