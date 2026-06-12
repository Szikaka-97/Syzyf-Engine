/////////////////////////////// Source file 0/////////////////////////////
#version 460

/////////////////////////////// Source file 1/////////////////////////////
#define _LINUX

/////////////////////////////// Source file 2/////////////////////////////
/////////////////////////////// Source file 0/////////////////////////////

/////////////////////////////// Source file 3/////////////////////////////

/////////////////////////////// Source file 1/////////////////////////////
#define _LINUX

/////////////////////////////// Source file 2/////////////////////////////
/////////////////////////////// Source file 0/////////////////////////////

/////////////////////////////// Source file 3/////////////////////////////

/////////////////////////////// Source file 1/////////////////////////////
#define _LINUX

/////////////////////////////// Source file 2/////////////////////////////
/////////////////////////////// Source file 0/////////////////////////////

/////////////////////////////// Source file 3/////////////////////////////

/////////////////////////////// Source file 1/////////////////////////////
#define _LINUX

/////////////////////////////// Source file 2/////////////////////////////
/////////////////////////////// Source file 0/////////////////////////////

/////////////////////////////// Source file 3/////////////////////////////

/////////////////////////////// Source file 1/////////////////////////////
#define _LINUX

/////////////////////////////// Source file 2/////////////////////////////

/////////////////////////////// Source file 3/////////////////////////////


/////////////////////////////// Source file 4/////////////////////////////

#line       1        1 
#ifndef SHADER_SHARED_H

#ifdef __cplusplus

#include <glm/glm.hpp>
#define vec3 glm::vec3
#define vec2 glm::vec2
#define t_bool unsigned int
#define uint unsigned int
#define mat4 glm::mat4

#else

#define t_bool uint

#endif

#ifndef __cplusplus

#define IN_POSITION location=0
#define IN_NORMAL location=1
#define IN_BINORMAL location=2
#define IN_TANGENT location=3
#define IN_UV1 location=4
#define IN_UV2 location=5
#define IN_COLOR location=6
#define IN_JOINTS location=7
#define IN_WEIGHTS location=8

#define POINT_LIGHT 0
#define SPOT_LIGHT 1
#define DIRECTIONAL_LIGHT 2

float inverseLerp(in float a, in float b, in float v) {
	return (v - a) / (b - a);
}

#endif

#ifdef __cplusplus
struct ShaderLightRep {
#else
struct Light {
#endif
	vec3 position;
	uint type;
	vec3 direction;
	float range;
	vec3 color;
	float spotlightAngle;
	float intensity;
	float linearAttenuation;
	float quadraticAttenuation;
	int shadowAtlasIndex;
};

struct ShadowMapRegion {
	mat4 viewTransform;
	vec2 start;
	vec2 end;
};

#ifdef uint
#undef uint
#endif

#ifdef vec3
#undef vec3
#endif

#ifdef vec2
#undef vec2
#endif

#ifdef mat4
#undef mat4
#endif

#undef t_bool

#define SHADER_SHARED_H
#endif

#line      6        0 
#line       1        2 
#ifndef SHADER_UNIFORMS_H

#ifdef __cplusplus

#include <glm/glm.hpp>

#define mat4 glm::mat4
#define mat3 glm::mat3x4
#define vec4 glm::vec4
#define vec3 alignas(vec4) glm::vec3
#define UNIFORM_DECL(bindingPoint) struct alignas(4 * sizeof(float))

#else

#define UNIFORM_DECL(bindingPoint) layout (std140, binding = bindingPoint) uniform

#endif

UNIFORM_DECL(0) ShaderGlobalUniforms
{
	mat4 Global_ViewMatrix;
	mat4 Global_InverseViewMatrix;
	mat4 Global_ProjectionMatrix;
	mat4 Global_InverseProjectionMatrix;
	mat4 Global_VPMatrix;
	vec4 Global_Resolution;
	vec3 Global_CameraWorldPos;
	float Global_Time;
    vec4 Global_PlayerWorldPos;
	float Global_CameraNearPlane;
	float Global_CameraFarPlane;
	float Global_CameraFov;
};
UNIFORM_DECL(1) ShaderObjectUniforms
{
	mat4 Object_ModelMatrix;
	mat4 Object_InverseModelMatrix;
	mat4 Object_MVPMatrix;
	mat3 Object_NormalModelMatrix;
};

#ifdef mat4
#undef mat4
#endif

#ifdef mat3
#undef mat3
#endif

#ifdef vec4
#undef vec4
#endif

#ifdef vec3
#undef vec3
#endif

#ifdef vec4
#undef vec4
#endif

#ifdef UNIFORM_DECL
#undef UNIFORM_DECL
#endif

#define SHADER_UNIFORMS_H
#endif

#line      7        0 

layout (IN_POSITION) in vec3 vPos;
layout (IN_NORMAL) in vec3 vNormal;

uniform mat4 targetCameraInverseProjectionMatrix;
uniform mat4 targetCameraInverseViewMatrix;
uniform mat4 targetCameraProjectionMatrix;
uniform vec2 targetCameraPlanes;
uniform vec3 gridDimentions;

out VS_OUT {
	vec3 worldPos;
	flat uint index;
	flat uint lightsAmount;
} vs_out;


layout (std430, binding = 9) restrict readonly buffer lightGridSSBO {
	uvec2 lightGrid[];
};

void main() {
	vec3 gridPos = vec3(
		mod(float(gl_InstanceID), gridDimentions.x),
		mod(floor(float(gl_InstanceID) / gridDimentions.x), gridDimentions.y),
		floor(float(gl_InstanceID) / (gridDimentions.x * gridDimentions.y))
	);

	vec3 position = vPos * 2;

	float vertZ = (position.z + 1) / 2;

	position += 1;

	position += gridPos;

	position /= (gridDimentions + 1) * 0.5;

	position.z = targetCameraPlanes.x * pow(targetCameraPlanes.y / targetCameraPlanes.x, (gridPos.z + vertZ) / float(gridDimentions.z));
	
	float back = position.z;

	vec4 temp = targetCameraProjectionMatrix * vec4(0, 0, -position.z, 1);

	position -= 1;

	position.z = temp.z / temp.w;

	vec4 viewPosition = targetCameraInverseProjectionMatrix * vec4(position, 1.0);

	viewPosition /= viewPosition.w;

	vec3 worldPosition = (targetCameraInverseViewMatrix * vec4(viewPosition.xyz, 1.0)).xyz;

	vs_out.worldPos = worldPosition;

	gl_Position = Global_VPMatrix * vec4(worldPosition, 1.0);

	vs_out.index = gl_InstanceID;
	vs_out.lightsAmount = lightGrid[gl_InstanceID].y;
}
