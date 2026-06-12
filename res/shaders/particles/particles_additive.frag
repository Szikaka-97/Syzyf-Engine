#version 460

#pragma additive

#include "particles/shared/frag_inputs.glsl"
#include "particles/shared/linearize_depth.glsl"

#define SHADING_LAMBERT
#include "shared/shading.h"
#include "shared/light.h"

out vec4 FragColor;

void main() {
    vec4 baseColor = texture(colorTex, ps_in.texcoords) * color;
    
    if (useColorRamp > 0) {
        vec4 rampColor = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        baseColor.rgb *= rampColor.rgb * colorIntensity;
    }

    float alpha = baseColor.a * ps_in.alpha;

    #include "particles/shared/fade.glsl"

    FragColor = vec4(baseColor.rgb * alpha, alpha);
}
