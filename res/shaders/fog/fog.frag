#version 460

#include "shared/uniforms.h"

in vec2 pUVCoords;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

uniform uint fogType;
uniform float maxDistance;
uniform float minDistance;
uniform float density;
uniform vec4 fogColor;

out vec4 fragColor;

void main() {
    vec4 color = texture(colorTex, pUVCoords);
    float dist = texture(depthTex, pUVCoords).x * 2.0 - 1.0;
    dist = (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) / (Global_CameraFarPlane + Global_CameraNearPlane - dist * (Global_CameraFarPlane - Global_CameraNearPlane));
    
    float factor = 1.0;

    if (fogType == 0) {
        factor = (maxDistance - dist) / (maxDistance - minDistance);
    } else if (fogType == 1) {
        factor = exp(-density * dist);
    } else if (fogType == 2) {
        factor = exp(-pow(density * dist, 2.0));
    }

    factor = clamp(factor, 0.0, 1.0);
    fragColor = mix(fogColor, color, factor);
}
