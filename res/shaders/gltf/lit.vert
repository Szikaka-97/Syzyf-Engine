#version 460

#include "gltf/shared/pbr_vert_inputs.glsl"

void main() {
	gl_Position = Object_MVPMatrix * vec4(vPos, 1.0);

	vs_out.worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
	vs_out.viewPos = (Global_ViewMatrix * (Object_ModelMatrix * vec4(vPos, 1.0))).xyz;
	vs_out.normal = Object_NormalModelMatrix * vNormal;
	vs_out.tangent.xyz = Object_NormalModelMatrix * vTangent.xyz;
	vs_out.tangent.w = vTangent.w;
	vs_out.texcoords = vUVCoords;
	vs_out.texcoords2 = vUVCoords2;
}
