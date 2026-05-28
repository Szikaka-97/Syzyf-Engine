#version 460
in vec3 viewNormal;
in vec2 pTexCoords;

layout (location = 1) out vec3 outNormal;

layout(location = 0) uniform vec4 baseColorFactor;
layout(location = 1) uniform float alphaCutoff;
layout(location = 2) uniform sampler2D albedoMap;

void main() {
    float baseAlpha = baseColorFactor.a == 0.0 ? 1.0 : baseColorFactor.a;
    float alpha = texture(albedoMap, pTexCoords).a * baseAlpha;

    if (alpha < alphaCutoff) {
        discard;
    }

    outNormal = normalize(viewNormal);
}
