#version 460

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;
layout (IN_TANGENT) in vec3 vTangent;
layout (IN_UV1) in vec2 vUVCoords;

out VS_OUT {
	vec3 worldPos;
	vec3 viewPos;
	vec3 normal;
	vec3 tangent;
	vec2 texcoords;
} vs_out;

uniform uint billboardMode;

void main() {
    if (billboardMode > 0) {
        vec3 worldCameraRight = vec3(
            Global_ViewMatrix[0][0],
            Global_ViewMatrix[1][0],
            Global_ViewMatrix[2][0]
        );
        vec3 worldCameraUp = vec3(
            Global_ViewMatrix[0][1],
            Global_ViewMatrix[1][1],
            Global_ViewMatrix[2][1]
        );
        vec3 worldCameraForward = vec3(
            Global_ViewMatrix[0][2],
            Global_ViewMatrix[1][2],
            Global_ViewMatrix[2][2]
        );

        // Z billboard
        if (billboardMode == 2) {
            worldCameraUp = vec3(0.0, 1.0, 0.0);
            worldCameraForward = normalize(cross(worldCameraRight, worldCameraUp));
        }

        vec3 center = Object_ModelMatrix[3].xyz;

        vec3 scale = vec3(
            length(Object_ModelMatrix[0].xyz),
            length(Object_ModelMatrix[1].xyz),
            length(Object_ModelMatrix[2].xyz)
        );

        vs_out.worldPos = center 
            + worldCameraRight * (vPos.x * scale.x)
            + worldCameraUp * (vPos.y * scale.y)
            + worldCameraForward * (vPos.z * scale.z);

        vs_out.normal = cross(worldCameraRight, worldCameraUp);
    } else {
	    vs_out.worldPos = (Object_ModelMatrix * vec4(vPos, 1.0)).xyz;
	    vs_out.normal = Object_NormalModelMatrix * vNormal;
    }

	gl_Position = Global_VPMatrix * vec4(vs_out.worldPos, 1.0);
	vs_out.viewPos = (Global_ViewMatrix * vec4(vs_out.worldPos, 1.0)).xyz;
	vs_out.tangent = Object_NormalModelMatrix * vTangent;
	vs_out.texcoords = vUVCoords;
}
