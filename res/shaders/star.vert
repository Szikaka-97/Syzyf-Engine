#version 460 core

#include "shared/shared.h"
#include "shared/uniforms.h"

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;

float rand(vec2 co) {
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);

    return fract(sin(sn) * c);
}

out vec3 pNormal;
out vec3 pWorldPos;

void main() {
	vec3 randomDisplacement = (vec3(
		rand(vec2(gl_InstanceID, 0)) - 0.5,
		rand(vec2(0, gl_InstanceID)),
		rand(vec2(gl_InstanceID, gl_InstanceID)) - 0.5
	)) * 3000;

	float rotationAngle = rand(vec2(gl_InstanceID, 0)) * 6.283;

	mat3 rotation;
	rotation[0] = vec3(cos(rotationAngle), 0, -sin(rotationAngle));
	rotation[1] = vec3(0, 1, 0);
	rotation[2] = vec3(sin(rotationAngle), 0, cos(rotationAngle));

	vec3 rotatedPos = rotation * vPos;

	gl_Position = Global_VPMatrix * vec4(rotatedPos + randomDisplacement, 1.0);
	pNormal = rotatedPos * vNormal;
	pWorldPos = rotatedPos + randomDisplacement;
	gl_Position.z = clamp(gl_Position.z, -1.0, 1.0);
	gl_Position.z *= gl_Position.z;
}