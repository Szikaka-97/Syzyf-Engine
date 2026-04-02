#version 460

#include "shared/uniforms.h"

in vec2 pUVCoords;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

out vec4 fragColor;

float LinearizeDepth(float depth) {
  float z = depth * 2.0 - 1.0;
  return (2.0 * Global_CameraNearPlane * Global_CameraFarPlane) /
    (Global_CameraFarPlane + Global_CameraNearPlane - z * (Global_CameraFarPlane - Global_CameraNearPlane));
}

void main() {
  float depthRaw = texture(depthTex, pUVCoords).r;
  float depth = LinearizeDepth(depthRaw);

  vec2 fogTexelSize = 1.0 / vec2(textureSize(colorTex, 0));
  vec2 fogUV = pUVCoords * vec2(textureSize(colorTex, 0)) - 0.5;
  vec2 index = floor(fogUV);
  vec2 fraction = fract(fogUV);

  vec2 uv00 = (index + vec2(0.5, 0.5)) * fogTexelSize;
  vec2 uv10 = (index + vec2(1.5, 0.5)) * fogTexelSize;
  vec2 uv01 = (index + vec2(0.5, 1.5)) * fogTexelSize;
  vec2 uv11 = (index + vec2(1.5, 1.5)) * fogTexelSize;

  vec4 fog00 = texture(colorTex, uv00);
  vec4 fog10 = texture(colorTex, uv10);
  vec4 fog01 = texture(colorTex, uv01);
  vec4 fog11 = texture(colorTex, uv11);

  float z00 = LinearizeDepth(texture(depthTex, uv00).r);
  float z10 = LinearizeDepth(texture(depthTex, uv10).r);
  float z01 = LinearizeDepth(texture(depthTex, uv01).r);
  float z11 = LinearizeDepth(texture(depthTex, uv11).r);

  vec4 spatialW = vec4(
    (1.0 - fraction.x) * (1.0 - fraction.y),
    fraction.x * (1.0 - fraction.y),
    (1.0 - fraction.x) * fraction.y,
    fraction.x * fraction.y
  );

  float depthTolerance = 1.5;
  vec4 depthDiff = vec4(
    abs(depth - z00),
    abs(depth - z10),
    abs(depth - z01),
    abs(depth - z11)
  );
  vec4 depthW = exp(-depthDiff * depthTolerance);

  vec4 finalWeights = spatialW * depthW;
  float sumWeights = dot(finalWeights, vec4(1.0));

  if (sumWeights < 0.0001) {
    finalWeights = vec4(0.25);
    sumWeights = 1.0;
  }

  vec4 finalFog = (fog00 * finalWeights.x +
      fog10 * finalWeights.y +
      fog01 * finalWeights.z +
      fog11 * finalWeights.w) / sumWeights;

  fragColor = finalFog;
}
