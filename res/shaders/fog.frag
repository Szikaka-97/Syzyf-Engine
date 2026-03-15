#version 460

in vec2 pUVCoords;

uniform sampler2D colorTex;
uniform sampler2D depthTex;

out vec4 fragColor;

void main() {
  // pass as uniforms
  float near = 0.1;
  float far = 100.0;

  float maxDist = 3.0;
  float minDist = 0.1;
  vec4 fogColor = vec4(0.4, 0.4, 0.4, 0.4);

  vec4 color = texture(colorTex, pUVCoords);
  float dist = texture(depthTex, pUVCoords).x * 2.0 - 1.0;
  dist = (2.0 * 0.1 * 100.0) / (100.0 + 0.1 - dist * (100.0 - 0.1));

  float factor = (maxDist - dist) /
                 (maxDist - minDist);
  factor = clamp(factor, 0.0, 1.0);

  fragColor = mix(fogColor, color, factor);
}
