#version 460

#pragma transparent

#include "gltf/shared/pbr_frag_inputs.glsl"
#include "gltf/shared/pbr_normal.glsl"

float calcWeight(float alpha) {
	return clamp(
		pow(min(1.0, alpha * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0),
		0.001,
		3000.0
	);
}

void main() {
  Material mat;
  vec2 texCoords = ps_in.texcoords;

  vec4 albedo = texture(albedoMap, texCoords);
  mat.albedo = albedo.xyz * baseColorFactor.xyz;
  float alpha = albedo.a * baseColorFactor.a;

  vec3 arm = texture(armMap, texCoords).xyz;
  vec3 N = getNormalFromMap();
  vec3 V = normalize(Global_CameraWorldPos - ps_in.worldPos);
  vec3 R = reflect(-V, N); 

  #include "gltf/shared/pbr_lighting.glsl"
}
