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

out VARYINGS {
	vec3 normal;
	vec3 worldPos;
	flat uint instanceID;
} vs_out;

void main() {
	float displacementAngleY = rand(vec2(gl_InstanceID, 0)) * 6.283 + Global_Time * 0.2;
	float displacementAngleX = rand(vec2(0, gl_InstanceID)) * 6.283;

	float displacementAmount = 8 + rand(vec2(gl_InstanceID, gl_InstanceID)) * 5;

	vec3 randomDisplacement = vec3(
		cos(displacementAngleY) * cos(displacementAngleX),
		sin(displacementAngleX),
		sin(displacementAngleY) * cos(displacementAngleX)
	) * displacementAmount;

	float rotationAngle = rand(vec2(gl_InstanceID, 0)) * 6.283 + (rand(vec2(0, vec2(gl_InstanceID, 0))) - 0.5) * Global_Time;

	mat3 rotation;
	rotation[0] = vec3(cos(rotationAngle), 0, -sin(rotationAngle));
	rotation[1] = vec3(0, 1, 0);
	rotation[2] = vec3(sin(rotationAngle), 0, cos(rotationAngle));

	vec3 rotatedPos = (rotation * (vPos * 0.1)) + randomDisplacement;

	gl_Position = Object_MVPMatrix * vec4(rotatedPos, 1);
	vs_out.normal = (Object_ModelMatrix * vec4(rotation * vNormal, 0)).xyz;
	vs_out.worldPos = (Object_ModelMatrix * vec4(rotatedPos, 1)).xyz;
	vs_out.instanceID = gl_InstanceID;
}