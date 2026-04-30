#version 460 core

layout (location = 1) out float FragColor;

#include "shared/uniforms.h"
#include "shared/shared.h"

in vec2 pUVCoords;

uniform sampler2D ssaoTex;
uniform int blurRange;

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssaoTex, 0));
    float result = 0.0;

    float sampleCount = float(blurRange * 2) * float(blurRange * 2);

    for (int x = -blurRange; x < blurRange; ++x) {
        for (int y = -blurRange; y < blurRange; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(ssaoTex, pUVCoords + offset).r;
        }
    }

    FragColor = result / sampleCount;
}
