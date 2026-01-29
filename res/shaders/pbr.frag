#version 460

in VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} ps_in;

#include "shared/shared.h"
#include "shared/uniforms.h"

#define SHADING_PBR

#include "shared/shading.h"

#include "shared/light.h"

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D armMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

vec3 getNormalFromMap() {
	vec3 tangentNormal = texture(normalMap, ps_in.texcoords).xyz * 2.0 - 1.0;

	vec3 N   = normalize(ps_in.normal);
	vec3 T  = normalize(ps_in.tangent);
	vec3 B  = -normalize(cross(N, T));
	mat3 TBN = mat3(T, B, N);

	return normalize(TBN * tangentNormal);
}

out vec4 fragColor;

void main() {
	Material mat;

	vec3 arm = texture(armMap, ps_in.texcoords).xyz;

	mat.albedo = texture(albedoMap, ps_in.texcoords).xyz;
	
	mat.metallic = arm.z;
	mat.roughness = max(arm.y, 0.05);
	float ao = arm.x;

	vec3 N = getNormalFromMap();
	vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);
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

	vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse = irradiance * mat.albedo;

	const float MAX_REFLECTION_LOD = 7.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, mat.roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), mat.roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * ao;

	fragColor.xyz += ambient;
}