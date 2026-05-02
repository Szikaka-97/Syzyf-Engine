#version 460
in vec3 viewNormal;
in vec2 pTexCoords;

layout (location = 1) out vec3 outNormal;

uniform sampler2D albedoMap;
uniform vec4 baseColorFactor;
uniform float alphaCutoff;

void main() {
    float alpha = texture(albedoMap, pTexCoords).a * baseColorFactor.a;

    if (alpha < alphaCutoff) {
        discard;
    }

    outNormal = normalize(viewNormal);
}
