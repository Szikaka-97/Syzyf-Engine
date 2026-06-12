#version 460

#pragma alpha_mask
#pragma dither_proximity

uniform float alphaCutoff;
uniform sampler2D ditherTex;

#include "gltf/shared/pbr_frag_inputs.glsl"
#include "gltf/shared/pbr_normal.glsl"

void main() {
	float distToCamera = distance(Global_CameraWorldPos.xyz, ps_in.worldPos);
	float fadeThreshold = smoothstep(1.0, 3.0, distToCamera);
	
	ivec2 texSize = textureSize(ditherTex, 0);
	ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % texSize;
	float ditherVal = texelFetch(ditherTex, ditherCoord, 0).r;
	
	if (fadeThreshold < ditherVal) {
		discard;
	}

	Material mat;
	vec2 texCoords = ps_in.texcoords;
	vec4 albedo = texture(albedoMap, texCoords);
	
	if (albedo.w < alphaCutoff) {
		discard;
	}

	mat.albedo = albedo.xyz * baseColorFactor.xyz;
	float alpha = 1.0;

	vec3 arm = texture(armMap, texCoords).xyz;
	vec3 N = getNormalFromMap();
	vec3 V = normalize(Global_CameraWorldPos.xyz - ps_in.worldPos);
	vec3 R = reflect(-V, N);

	#include "gltf/shared/pbr_lighting.glsl"
}
