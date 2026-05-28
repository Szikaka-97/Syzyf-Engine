#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform uint interactionState; 
uniform float level;

uniform vec4 highColor;
uniform vec4 lowColor;
uniform vec4 backgroundColor;

const float PI = 3.14159265359;

void main() {
    vec2 centeredUv = TexCoords * 2.0 - 1.0;

    float radius = length(centeredUv);

    if (radius > 1.0 || radius < 0.7) {
        discard; 
    }

    float angle = atan(centeredUv.x, centeredUv.y);
    float normalizedAngle = (angle + PI) / (2.0 * PI);

    normalizedAngle = 1.0 - normalizedAngle;

    vec4 finalColor = mix(lowColor, highColor, level);

    if (normalizedAngle > level) {
        finalColor = backgroundColor;
    }

    if (interactionState == 1) {
        finalColor.rgb += 0.15;
    } else if (interactionState == 2) {
        finalColor.rgb -= 0.15;
    }

    FragColor = finalColor;
}
