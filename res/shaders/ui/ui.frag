#version 460 core

in vec2 TexCoords;

out vec4 FragColor;

uniform vec4 color;
uniform sampler2D tex;
uniform bool hasTexture;

void main() {
    vec4 finalColor = color;

    if (hasTexture) {
        finalColor *= texture(tex, TexCoords);
    }

    if (finalColor.a < 0.01) {
        discard;
    }

    FragColor = finalColor;
}
