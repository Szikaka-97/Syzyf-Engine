#version 460

in vec2 pUVCoords;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

uniform float near;
uniform float far;
uniform float maxDistance;
uniform float minDistance;
uniform vec4 fogColor;

out vec4 fragColor;

void main() {
  vec4 color = texture(colorTex, pUVCoords);
  float dist = texture(depthTex, pUVCoords).x * 2.0 - 1.0;
  dist = (2.0 * near * far) / (far + near - dist * (far - near));

  float factor = (maxDistance - dist) /
                 (maxDistance - minDistance);
  factor = clamp(factor, 0.0, 1.0);

  fragColor = mix(fogColor, color, factor);
}
