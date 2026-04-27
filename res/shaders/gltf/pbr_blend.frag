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

layout (location = 0) out vec4 accumValue;
layout (location = 1) out float revealValue;

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

float calcWeight(float alpha) {
	return clamp(
		pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
		0.001,
		3000.0
	);
}

vec3 getNormalFromMap() {
	vec3 tangentNormal = texture(normalMap, ps_in.texcoords).xyz * 2.0 - 1.0;

	vec3 N   = normalize(ps_in.normal);
	vec3 T  = normalize(ps_in.tangent.xyz);
	vec3 B  = normalize(cross(N, T)) * ps_in.tangent.w;
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

void main() {
	Material mat;

  // ALBEDO
  vec4 albedo = texture(albedoMap, ps_in.texcoords);
	
  mat.albedo = albedo.xyz * baseColorFactor.xyz;
  float alpha = albedo.a * baseColorFactor.a;

  vec3 arm = texture(armMap, ps_in.texcoords).xyz;

	mat.metallic = arm.b * metallicFactor;
	mat.roughness = arm.g * roughnessFactor;

  float ao = 1.0f;
  if (useOcclusion) {
    ao = arm.r;
  }

	vec3 N = getNormalFromMap();
  vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);
	vec3 R = reflect(-V, N); 

	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, mat.albedo, mat.metallic);

	vec4 finalColor = vec4(
		shade(
			mat,
			ps_in.worldPos,
			N,
			vec3(0, 0, 0)
		),
		alpha
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

	vec3 ambient = (kD * diffuse + specular) * ao;
	
  vec3 emissive = texture(emissiveMap, ps_in.texcoords).xyz * emissiveFactor * emissiveStrength;

  finalColor.xyz += ambient + emissive;

  const float weight = calcWeight(finalColor.a);
  const vec3 viewDir = normalize(Global_CameraWorldPos - ps_in.worldPos);

  accumValue = vec4(finalColor.rgb * finalColor.a, finalColor.a) * weight;
  revealValue = finalColor.a;
}
