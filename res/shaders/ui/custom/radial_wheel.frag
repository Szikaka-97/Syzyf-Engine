#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform uint interactionState; 
uniform float level;

uniform uint hoveredSlice;
uniform uint numberOfSlices;

uniform vec4 color;

const float PI = 3.14159265359;
const float TWO_PI = 6.28318530718;
const float ALPHA_MUT = 0.3;


// change all hardcoded values to uniforms 
void main() {
    vec2 centeredUv = TexCoords * 2.0 - 1.0;
    float radius = length(centeredUv);

    if (radius > 1.0 || radius < 0.2) {
        discard;
    }

    float angle = atan(centeredUv.y, centeredUv.x);
    if (angle < 0.0) {
        angle += TWO_PI;
    }

    float sliceAngle = TWO_PI / float(numberOfSlices);
    int currentSlice = int(angle / sliceAngle);

    float gapWidth = 0.02;
    float angularGap = gapWidth / radius;

    float localSliceAngle = mod(angle, sliceAngle);
    if (localSliceAngle < angularGap || localSliceAngle > sliceAngle - angularGap) {
        discard;
    }

    vec4 finalColor = color;
    finalColor.a = clamp(finalColor.a * ALPHA_MUT, 0.0, 1.0);
    if (currentSlice == hoveredSlice) {
        finalColor.rgb += vec3(2.0);
    }

    float edgeSoftness = fwidth(radius);
    float circleMask = 1.0 - smoothstep(1.0 - edgeSoftness, 1.0 + edgeSoftness, radius);

    if (circleMask <= 0.0) {
        discard; 
    }

    finalColor.a *= circleMask;

    FragColor = finalColor;
}
