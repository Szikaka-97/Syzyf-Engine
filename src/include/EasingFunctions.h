// https://easings.net
#pragma once

#include <cmath>

namespace Easing {
  
  inline float inOutSine(const float x) {
    return -(cos(M_PI * x) - 1.0f) * 0.5f;
  }

  inline float inOutBack(const float x) {
    const float c1 = 1.70158f;
    const float c2 = c1 * 1.525f;

    if (x < 0.5f) {
      return ((2.0f * x) * (2.0f * x) * ((c2 + 1.0f) * 2.0f * x - c2)) * 0.5f;
    } else {
      return ((2.0f * x - 2.0f) * (2.0f * x - 2.0f) * ((c2 + 1.0f) * (x * 2.0f - 2.0f) + c2) + 2.0f) * 0.5f;
    }
  }

  inline float outBounce(float x) {
    const float n1 = 7.5625f;
    const float d1 = 2.75f;

    if (x < 1.0f / d1) {
      return n1 * x * x;
    } else if (x < 2.0f / d1) {
      x -= 1.5f / d1;
      return n1 * x * x + 0.75f;
    } else if (x < 2.5f / d1) {
      x -= 2.25f / d1;
      return n1 * x * x + 0.9375f;
    } else {
      x -= 2.625f / d1;
      return n1 * x * x + 0.984375f;
    }
  }
}
