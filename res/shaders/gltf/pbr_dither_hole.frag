#version 460

#pragma alpha_mask
#pragma dither_hole

uniform float alphaCutoff;
uniform sampler2D ditherTex;

#include "gltf/shared/pbr_frag_inputs.glsl"
#include "gltf/shared/pbr_normal.glsl"

void main() {
	if (Global_PlayerWorldPos.w > 0.5) {
		vec3 camToPlayer = Global_PlayerWorldPos.xyz - Global_CameraWorldPos.xyz;
		float distToPlayer = length(camToPlayer);
		vec3 dirToPlayer = camToPlayer / distToPlayer;

		vec3 camToFrag = ps_in.worldPos - Global_CameraWorldPos.xyz;
		float t = dot(camToFrag, dirToPlayer);

		if (t > 0.0 && t < distToPlayer) {
			float distToLine = length(camToFrag - dirToPlayer * t);
			float holeRadius = 1.5; 
			float fadeWidth = 1.0;

			if (distToLine < holeRadius + fadeWidth) {
				float alphaMask = smoothstep(holeRadius, holeRadius + fadeWidth, distToLine);
				ivec2 texSize = textureSize(ditherTex, 0);
				ivec2 ditherCoord = ivec2(gl_FragCoord.xy) % texSize;
				float ditherVal = texelFetch(ditherTex, ditherCoord, 0).r;
				
				if (alphaMask < ditherVal) {
					discard;
				}
			}
		}
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
