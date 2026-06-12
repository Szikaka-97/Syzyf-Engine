#version 460

#include "particles/shared/frag_inputs.glsl"

#define SHADING_LAMBERT
#include "shared/shading.h"
#include "shared/light.h"

layout (location = 0) out vec4 FragColor;

void main() {
    vec4 texColor = texture(colorTex, ps_in.texcoords) * color;
    
    if (useColorRamp > 0) {
        vec4 colorRampTex = texture(colorRamp, vec2(ps_in.lifetime, 0.0));
        texColor.rgb *= colorRampTex.rgb * 5.0;
    }

    const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);
    FragColor = vec4(texColor.rgb * dot(viewDir, normalize(ps_in.normal)), 1.0);
}
