#version 460 core

layout (location = 1) out float FragAO;
layout (location = 2) out vec3 FragSSGI;

#include "shared/uniforms.h"
#include "shared/shared.h"

in vec2 pUVCoords;

uniform sampler2D depthTex;
uniform sampler2D normalTex;
uniform sampler2D noiseTex;
uniform sampler2D colorTex;

uniform vec2 resolution;
uniform float radius;
uniform float bias;
uniform float power;
uniform int raySteps;
uniform float ssgiIntensity;

const float PI = 3.14159265359;
const int NUM_DIRECTIONS = 6; 

vec3 ViewPosFromDepth(vec2 uv, float rawDepth) {
    vec4 ndc = vec4(uv * 2.0 - 1.0, rawDepth * 2.0 - 1.0, 1.0);
    vec4 viewPos = Global_InverseProjectionMatrix * ndc;
    return viewPos.xyz / viewPos.w;
}

void main() {
    float rawDepth = texture(depthTex, pUVCoords).r;
    if (rawDepth >= 1.0) {
        FragAO = 1.0;
        FragSSGI = vec3(0.0);
        return;
    }

    vec3 fragPos = ViewPosFromDepth(pUVCoords, rawDepth);
    vec3 normal = normalize(texture(normalTex, pUVCoords).rgb);

    vec2 noiseScale = resolution / 4.0;
    vec3 randomVec = normalize(texture(noiseTex, pUVCoords * noiseScale).xyz);

    float ao = 0.0;
    vec3 ssgi = vec3(0.0);

    int numSteps = max(raySteps / NUM_DIRECTIONS, 4);

    float uvRadius = (radius * 0.5) / max(abs(fragPos.z), 0.001);
    
    uvRadius = min(uvRadius, 0.2); 
    
    float stepSize = uvRadius / float(numSteps);
    float aspect = resolution.x / resolution.y;

    for (int i = 0; i < NUM_DIRECTIONS; ++i) {
        float angle = (float(i) / float(NUM_DIRECTIONS)) * 2.0 * PI;
        vec2 dir = vec2(cos(angle), sin(angle));
        
        vec2 rotatedDir = vec2(
            dir.x * randomVec.x - dir.y * randomVec.y,
            dir.x * randomVec.y + dir.y * randomVec.x
        );

        rotatedDir.y *= aspect;

        float maxHorizon = bias; 

        for (int j = 1; j <= numSteps; ++j) {
            vec2 sampleUV = pUVCoords + rotatedDir * (stepSize * float(j));
            
            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) break;

            float sampleDepth = texture(depthTex, sampleUV).r;
            vec3 samplePos = ViewPosFromDepth(sampleUV, sampleDepth);
            
            vec3 delta = samplePos - fragPos;
            float dist = length(delta);
            vec3 sampleDir = delta / max(dist, 0.0001);

            float elevation = dot(normal, sampleDir);

            if (elevation > maxHorizon && dist < radius) {
                float weight = smoothstep(0.0, 1.0, 1.0 - (dist / radius));
                
                ao += (elevation - max(maxHorizon, 0.0)) * weight;

                vec3 bounceColor = texture(colorTex, sampleUV).rgb;

                float luminance = dot(bounceColor, vec3(0.2126, 0.7152, 0.0722));

                float maxLuminance = 1.5;
                if (luminance > maxLuminance) {
                    bounceColor *= (maxLuminance / luminance);
                }
                
                ssgi += bounceColor * elevation * weight;

                maxHorizon = elevation;
            }
        }
    }

    ao = 1.0 - (ao / float(NUM_DIRECTIONS));
    FragAO = pow(clamp(ao, 0.0, 1.0), power);
    
    FragSSGI = (ssgi / float(NUM_DIRECTIONS)) * ssgiIntensity;
}
