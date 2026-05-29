#version 460

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

uniform float heightScale;
uniform float pomMinLayers;
uniform float pomMaxLayers;

uniform vec2 uvScale;

out vec4 fragColor;

vec3 getNormalFromMap(vec2 uv, mat3 TBN) {
	vec3 tangentNormal = texture(normalMap, uv).xyz * 2.0 - 1.0;
	return normalize(TBN * tangentNormal);
}

vec2 parallaxMapping(vec2 texCoords, vec3 viewDir) {
    float numLayers = mix(pomMaxLayers, pomMinLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * heightScale;
    vec2 deltaTexCoords = P / numLayers;

    vec2 currentTexCoords = texCoords;
    float currentDepthMapValue = texture(armMap, currentTexCoords * uvScale).a;

    while (currentLayerDepth < currentDepthMapValue) {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = texture(armMap, currentTexCoords * uvScale).a;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = texture(armMap, prevTexCoords * uvScale).a - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main() {
    vec2 texCoords = ps_in.texcoords;
    
    vec3 N_World = normalize(ps_in.normal);
    vec3 T = normalize(ps_in.tangent.xyz);
    T = normalize(T - dot(T, N_World) * N_World);
    vec3 B = cross(N_World, T) * ps_in.tangent.w;
    mat3 TBN = mat3(T, B, N_World);
    vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);

    if (heightScale > 0.0) {
        vec3 V_Tangent = normalize(vec3(
            dot(V, T),
            dot(V, B),
            dot(V, N_World)
        ));

        texCoords = parallaxMapping(ps_in.texcoords, V_Tangent);
        // if (texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0) {
        //     discard;
        // }
    }

    texCoords *= uvScale;

	Material mat;

  // ALBEDO
  vec4 albedo = texture(albedoMap, texCoords);

  // Ignores alpha for now
	mat.albedo = albedo.xyz * baseColorFactor.xyz;

  vec3 arm = texture(armMap, texCoords).xyz;

	mat.metallic = arm.b * metallicFactor;
	mat.roughness = arm.g * roughnessFactor;

  // Ambient Occlusion
  float ao = 1.0f;

  vec2 screenUV = gl_FragCoord.xy / Global_Resolution.xy; 
  float ssao = texture(Builtin_AOMap, screenUV).r;

  if (useOcclusion) {
    ao = arm.r;
  }
  ao = ao * ssao;

	vec3 N = getNormalFromMap(texCoords, TBN);
	vec3 R = reflect(-V, N); 

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, mat.albedo, mat.metallic);

	fragColor = vec4(
		shade(
			mat,
			ps_in.worldPos,
			N,
			vec3(0, 0, 0)
		),
		1.0
	);

	vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, mat.roughness);

	vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - mat.metallic;

	vec3 irradiance = texture(Builtin_EnvIrradianceMap, N).rgb;
    vec3 diffuse = irradiance * mat.albedo;

	const float MAX_REFLECTION_LOD = 7.0;
    vec3 prefilteredColor = textureLod(Builtin_EnvPrefilterMap, R, mat.roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf = texture(Builtin_BRDFConvolutionMap, vec2(max(dot(N, V), 0.0), mat.roughness)).rg;
	
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao * (Light_AmbientLight.xyz * Light_AmbientLight.w);

  vec3 emissive = texture(emissiveMap, texCoords).xyz * emissiveFactor * emissiveStrength;

  fragColor.xyz += ambient + emissive;
}
