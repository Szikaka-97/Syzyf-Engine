#version 460

#include "particles/shared/frag_inputs.glsl"

#define SHADING_LAMBERT
#include "shared/shading.h"
#include "shared/light.h"

uniform sampler2D ditherTex;
layout (location = 0) out vec4 FragColor;

const int DITHER_TEX_SIZE = 16;

void main() {
    vec4 texColor = texture(colorTex, ps_in.texcoords) * color;
    
    if (useColorRamp > 0) {
        vec4 colorRampTex = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        texColor.rgb *= colorRampTex.rgb * 5.0;
    }

    float alpha = texColor.a * ps_in.alpha;
    
    #include "particles/shared/fade.glsl"

    ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % DITHER_TEX_SIZE;
    float ditherThreshold = texelFetch(ditherTex, ditherCoord, 0).r;
    
    if (alpha < ditherThreshold) {
        discard;
    }
    
    const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);
    FragColor = vec4(texColor.rgb * dot(viewDir, normalize(ps_in.normal)), 1.0);
}
