#version 460

in VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

#define SHADING_LAMBERT
#include "shared/shading.h"
#include "shared/light.h"

uniform vec3 uColor;
uniform vec3 rimColor;
uniform float rimPower;
uniform float rimStrength;

out vec4 fragColor;

void main()
{
    vec3 N = normalize(ps_in.normal);

    Material mat = defaultMaterial();
    mat.diffuseColor = uColor;
    mat.diffuseStrength = 1.0;

    // Normalne oświetlenie sceny z silnika, czyli światła + ambient + cienie
    vec3 baseLighting = shade(
    mat,
    ps_in.worldPos,
    N,
    vec3(0.0)
    );

    vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);

    float rim = 1.0 - max(dot(N, V), 0.0);
    rim = pow(rim, rimPower);

    vec3 rimLighting = rim * rimColor * rimStrength;

    vec3 finalColor = baseLighting + rimLighting;

    fragColor = vec4(finalColor, 1.0);
}