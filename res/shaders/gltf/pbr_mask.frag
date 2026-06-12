#version 460

#pragma alpha_mask

uniform float alphaCutoff;

#include "gltf/shared/pbr_frag_inputs.glsl"
#include "gltf/shared/pbr_normal.glsl"

void main() {
	Material mat;
	vec2 texCoords = ps_in.texcoords;

  // ALBEDO
  vec4 albedo = texture(albedoMap, texCoords);
  
  if (albedo.w < alphaCutoff) {
    discard;
  }

  mat.albedo = albedo.xyz * baseColorFactor.xyz;
  float alpha = 1.0;

  vec3 arm = texture(armMap, texCoords).xyz;
  vec3 N = getNormalFromMap();
  vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);
  vec3 R = reflect(-V, N); 

  #include "gltf/shared/pbr_lighting.glsl"
}
