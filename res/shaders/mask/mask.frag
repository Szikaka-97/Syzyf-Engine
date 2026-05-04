#version 460
layout (location = 1) out vec4 outColor;
uniform vec4 uMaskColor;

void main() {
    outColor = uMaskColor;
}
