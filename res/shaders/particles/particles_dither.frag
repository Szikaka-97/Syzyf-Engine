#version 460

in VS_OUT {
    vec3 worldPos;
    vec3 viewPos;
    vec3 normal;
    vec3 tangent;
    vec2 texcoords;
    float alpha;
    float lifetime;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

#define SHADING_LAMBERT

#include "shared/shading.h"
#include "shared/light.h"

uniform sampler2D colorTex;
uniform vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
uniform sampler2D colorRamp;
uniform uint useColorRamp;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;

uniform sampler2D ditherTex;

layout (location = 0) out vec4 FragColor;

const int DITHER_TEX_SIZE = 16;

void main() {
    vec4 texColor = texture(colorTex, ps_in.texcoords) * color;
    if (useColorRamp > 0) {
        vec4 colorRampTex = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        texColor.rgb *= colorRampTex.rgb * 5.0;
    }

    float finalAlpha = texColor.a * ps_in.alpha;

    if (proximityFadeMode > 0) {
        float distanceToCamera = length(ps_in.viewPos);
        finalAlpha *= smoothstep(proximityFadeMin, proximityFadeMax, distanceToCamera);
    }

    ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % DITHER_TEX_SIZE;
    float ditherThreshold = texelFetch(ditherTex, ditherCoord, 0).r;
    
    if (finalAlpha < ditherThreshold) {
        discard;
    }
    
    const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);
    FragColor = vec4(texColor.rgb * dot(viewDir, normalize(ps_in.normal)), 1.0);
}
