in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec4 tangent;
	vec2 texcoords;
	vec2 texcoords2;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

#define SHADING_PBR
#include "shared/shading.h"
#include "shared/light.h"

uniform vec4 baseColorFactor;
uniform sampler2D albedoMap;
uniform float roughnessFactor;
uniform float metallicFactor;
uniform sampler2D armMap;
uniform bool useOcclusion;
uniform sampler2D normalMap;
uniform vec3 emissiveFactor;
uniform float emissiveStrength;
uniform sampler2D emissiveMap;

uniform samplerCube Builtin_EnvIrradianceMap;
uniform samplerCube Builtin_EnvPrefilterMap;
uniform sampler2D Builtin_BRDFConvolutionMap;
uniform sampler2D Builtin_AOMap;
uniform sampler2D Builtin_SSGIMap;

out vec4 fragColor;
