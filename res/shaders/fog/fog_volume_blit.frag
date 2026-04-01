#version 460

in vec2 pUVCoords;

uniform sampler2D colorTex;

out vec4 fragColor;

void main() {
    fragColor = texture(colorTex, pUVCoords);
}
