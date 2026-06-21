#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform float percent;
uniform sampler2D tex;

void main() {
    if (TexCoords.x > percent) {
        discard;
    }

    vec4 color = texture(tex, TexCoords);

    FragColor = color + vec4(0.2, 0.0, 0.0, 0.0);
}
