#version 460

#pragma transparent 
#pragma no_cull

#include "particles/shared/frag_inputs.glsl"
#include "particles/shared/linearize_depth.glsl"

out vec4 fragColor;

void main() {
    vec4 texColor = texture(colorTex, ps_in.texcoords);
    
    if (texColor.r * 5.0 < ps_in.lifetime) {
        discard;
    }

    if (useColorRamp > 0) {
        vec4 colorRampTex = texture(colorRamp, vec2(clamp(1.0 - ps_in.lifetime, 0.2, 0.8), 0.0));
        texColor.rgb *= colorRampTex.rgb * 5.0;
    }

    float alpha = texColor.a * ps_in.alpha * 0.8;

    #include "particles/shared/fade.glsl"

    if (alpha < 0.001) discard;

    fragColor = vec4(texColor.rgb, alpha);
}
