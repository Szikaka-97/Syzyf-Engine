in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
    float alpha;
    float lifetime;
    flat uint index;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

uniform sampler2D colorTex;
uniform vec4 color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
uniform float colorIntensity = 1.0f;
uniform sampler2D colorRamp;
uniform uint useColorRamp;

uniform sampler2D depthTex;
uniform float depthFadeDistance;
uniform uint enableDepthFade;

uniform uint proximityFadeMode;
uniform float proximityFadeMin;
uniform float proximityFadeMax;
