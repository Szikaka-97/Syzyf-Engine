#version 460

#pragma transparent

#include "particles/shared/frag_inputs.glsl"
#include "particles/shared/linearize_depth.glsl"
#include "particles/shared/math.glsl"

#define SHADING_LAMBERT
#include "shared/shading.h"
#include "shared/light.h"

out vec4 FragColor;

void main() {
	vec4 texColor = texture(colorTex, ps_in.texcoords) * color;
    if (useColorRamp > 0) {
        vec4 colorRampTex = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        texColor.rgb *= colorRampTex.rgb * colorIntensity;
    }

    float alpha = texColor.a * ps_in.alpha;
    
    #include "particles/shared/fade.glsl"

    uint randomState = uint(ps_in.index);

    FragColor = vec4(texColor.rgb * hsl2rgb(random(randomState), 1.0f, 0.5f), alpha);
}
