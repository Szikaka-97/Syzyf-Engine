#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D msdfAtlas;
uniform vec4 textColor;
uniform float pxRange;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec3 msd = texture(msdfAtlas, TexCoords).rgb;
    float sd = median(msd.r, msd.g, msd.b);

    vec2 texSize = vec2(textureSize(msdfAtlas, 0));

    vec2 pxCoords = TexCoords * texSize;

    vec2 dx = dFdx(pxCoords);
    vec2 dy = dFdy(pxCoords);

    float toPixels = pxRange * inversesqrt(dot(dx, dx) + dot(dy, dy));

    float screenPxDistance = toPixels * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);

    if (opacity < 0.01) discard;

    FragColor = vec4(textColor.rgb, textColor.a * opacity);
}
