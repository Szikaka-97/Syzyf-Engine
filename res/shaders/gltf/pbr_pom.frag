#version 460

#include "gltf/shared/pbr_frag_inputs.glsl"
#include "gltf/shared/pbr_pom.glsl"

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

    vec4 albedo = texture(albedoMap, texCoords);

	mat.albedo = albedo.xyz * baseColorFactor.xyz;
	float alpha = 1.0;

	vec3 arm = texture(armMap, texCoords).xyz;
	vec3 N = getNormalFromMap(texCoords, TBN);
	vec3 R = reflect(-V, N); 

	#include "gltf/shared/pbr_lighting.glsl"
}
