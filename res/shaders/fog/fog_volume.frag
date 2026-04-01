#version 460

#include "shared/shared.h"
#include "shared/light.h"
#include "shared/uniforms.h"

in VS_OUT {
    vec3 worldPos;
} ps_in;

uniform sampler2D depthTex;

uniform float stepSize;
uniform float scatteringDensity;
uniform float absorptionDensity;
uniform vec3 scatteringColor;
uniform float k;
uniform float transmittanceThreshold;
uniform float bias;

uniform int intersectingLightCount;
uniform int intersectingLightIndices[128];

const float PI = 3.14159265359;

out vec4 fragColor;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(145.0, 7.233))) * 4274.84205094213454340);
}

vec3 FragmentWorldPos(float depthValue, vec2 uv) {
    float z = depthValue * 2.0 - 1.0;

    vec4 clipSpacePos = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 viewSpacePos = Global_InverseProjectionMatrix * clipSpacePos;

    viewSpacePos /= viewSpacePos.w;

    vec4 worldSpacePos = Global_InverseViewMatrix * viewSpacePos;

    return worldSpacePos.xyz;
}

float AbsorptionFactor(float density, float distance) {
    return exp(-distance * density);
}

float PhaseFunction_Schlick(vec3 w0, vec3 w1) {
    float cos_theta = dot(w0, w1);
    float nom = 1.0 - k * k;
    float denom = 4.0 * PI * (1.0 + k * cos_theta) * (1.0 + k * cos_theta);
    return nom / denom;
}

vec2 IntersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1.0 / (rayDir + 0.0000001);

    vec3 tMin = (boxMin - rayOrigin) * invDir;
    vec3 tMax = (boxMax - rayOrigin) * invDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

void main() {
    // pass the texture size as uniform instead
    vec2 texSize = vec2(textureSize(depthTex, 0)) * 0.5;
    vec2 screenUV = gl_FragCoord.xy / texSize;

    float z = texture(depthTex, screenUV).x;
    vec3 sceneFragPos = FragmentWorldPos(z, screenUV);
    float sceneDistance = length(sceneFragPos - Global_CameraWorldPos);

    vec3 rayDir = normalize(ps_in.worldPos - Global_CameraWorldPos);

    mat4 invModel = Object_InverseModelMatrix;
    vec3 rayOriginLocal = (invModel * vec4(Global_CameraWorldPos, 1.0)).xyz;
    vec3 rayDirLocal = normalize((invModel * vec4(rayDir, 0.0)).xyz);

    vec2 tLocal = IntersectAABB(rayOriginLocal, rayDirLocal, vec3(-0.5), vec3(0.5));

    vec3 worldDir = (Object_ModelMatrix * vec4(rayDirLocal, 0.0)).xyz;
    float scale = length(worldDir);

    float tNearWorld = tLocal.x * scale;
    float tFarWorld = tLocal.y * scale;

    float startDist = max(0.0, tNearWorld);
    float endDist = min(tFarWorld, sceneDistance);

    if (startDist >= endDist) {
        discard;
    }

    float rayDistance = endDist - startDist;

    // change to uniform
    const float MAX_STEPS = 64.0;
    float finalStepSize = max(stepSize, rayDistance / MAX_STEPS);
    vec3 deltaStep = rayDir * finalStepSize;

    vec3 marchPos = Global_CameraWorldPos + (rayDir * startDist);
    float rand = random(screenUV);
    marchPos += deltaStep * rand;

    vec3 viewRayDir = (Global_ViewMatrix * vec4(rayDir, 0.0)).xyz;
    vec3 viewRayOrigin = (Global_ViewMatrix * vec4(marchPos, 1.0)).xyz;

    vec3 fragToCameraNorm = -rayDir;
    vec3 radiance = vec3(0.0);
    float transmittance = 1.0;

    for (float l = 0; l < rayDistance; l += finalStepSize) {
        vec3 stepRadiance = vec3(0.0);

        float viewZ = viewRayOrigin.z + viewRayDir.z * l;
        float pixelDepth = max(0.0, -viewZ / Global_CameraFarPlane);
        uint dirCascadeIndex = uint(floor(sqrt(pixelDepth) * Light_DirectionalLightCascadeCount));

        for (int i = 0; i < intersectingLightCount; i++) {
            int lightIndex = intersectingLightIndices[i];

            Light light = Light_LightsList[lightIndex];
            if (light.intensity <= 0.0) {
                continue;
            }

            vec3 lightToPos;
            float lightDistance;
            uint shadowIndex = 0;

            if (light.type == DIRECTIONAL_LIGHT) {
              lightToPos = -light.direction;
              lightDistance = 1.0;
              shadowIndex = dirCascadeIndex;
            } else {
              lightToPos = marchPos - light.position;
              lightDistance = length(lightToPos);
              if (lightDistance > light.range) continue;

              vec3 lightDir = normalize(-lightToPos);
              if (abs(lightDir.x) > abs(lightDir.y) && abs(lightDir.x) > abs(lightDir.z)) {
                shadowIndex = lightDir.x > 0 ? 1 : 0;
              } else if (abs(lightDir.y) > abs(lightDir.z)) {
                shadowIndex = lightDir.y > 0 ? 3 : 2;
              } else {
                shadowIndex = lightDir.z > 0 ? 5 : 4;
              }
            }

            float visibility = 1.0;

            if (light.type == DIRECTIONAL_LIGHT) {
                lightToPos = -light.direction;
            }

            if (light.shadowAtlasIndex >= 0) {
                ShadowMapRegion mask = Light_ShadowMapRegions[light.shadowAtlasIndex + shadowIndex];
                vec4 lightViewPos = mask.viewTransform * vec4(marchPos, 1);
                lightViewPos /= lightViewPos.w;
                lightViewPos.z = (lightViewPos.z + 1) * 0.5;

                vec2 uvLocal = clamp(vec2((lightViewPos.x + 1) * 0.5, (lightViewPos.y + 1) * 0.5), 0, 1);
                vec2 uv = mix(mask.start, mask.end, uvLocal);

                float shadowZ = texture(Builtin_ShadowMask, uv).x;
                visibility = (lightViewPos.z - bias > shadowZ) ? 0.0 : 1.0;
            }

            vec3 lightStrength = getLightStrength(light, marchPos);

            vec3 Lin = AbsorptionFactor(absorptionDensity, lightDistance) * lightStrength * visibility;
            vec3 Li = Lin * scatteringDensity * scatteringColor * PhaseFunction_Schlick(normalize(lightToPos), fragToCameraNorm);

            stepRadiance += Li;
        }
        transmittance *= AbsorptionFactor(scatteringDensity + absorptionDensity, finalStepSize);
        radiance += stepRadiance * transmittance * finalStepSize;

        if (transmittance < transmittanceThreshold) {
            break;
        }

        marchPos += deltaStep;
    }

    fragColor = vec4(radiance, transmittance);
}
