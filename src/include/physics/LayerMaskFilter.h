#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Body/Body.h>

namespace Physics {
class LayerMaskFilter :public JPH::IgnoreMultipleBodiesFilter {
  private:
    uint32_t mask;
    bool isIncludeMode;

  public:
    LayerMaskFilter(uint32_t mask, bool isIncludeMode = true);

    LayerMaskFilter(std::initializer_list<uint32_t> layers, bool isIncludeMode = true);

    virtual bool ShouldCollideLocked(const JPH::Body& inBody) const override;
};
}
