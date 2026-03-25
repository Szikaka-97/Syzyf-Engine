#include "physics/LayerMaskFilter.h"

namespace Physics {
LayerMaskFilter::LayerMaskFilter(uint32_t mask, bool isIncludeMode)
      : mask(mask), isIncludeMode(isIncludeMode) {}

LayerMaskFilter::LayerMaskFilter(std::initializer_list<uint32_t> layers, bool isIncludeMode) : mask(0), isIncludeMode(isIncludeMode) {
  for (uint32_t layer : layers) {
    this->mask |= (1 << layer);
  }
}

bool LayerMaskFilter::ShouldCollideLocked(const JPH::Body& inBody) const {
  if (!JPH::IgnoreMultipleBodiesFilter::ShouldCollideLocked(inBody)) {
    return false;
  }

  uint32_t bodyLayerMask = inBody.GetCollisionGroup().GetGroupID();

  if (isIncludeMode) {
    return (bodyLayerMask & mask) != 0;
  } else {
    return (bodyLayerMask & mask) == 0;
  }
}
}
