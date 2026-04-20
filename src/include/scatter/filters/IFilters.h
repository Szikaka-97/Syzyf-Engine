#pragma once

#include <glm/glm.hpp>

namespace Scatter {

struct InstanceData;

using PointStream = std::vector<glm::vec3>;
using InstanceStream = std::vector<InstanceData>;

class IPointFilter {
public:
    virtual ~IPointFilter() = default;
    virtual PointStream Process(const PointStream& input) = 0;
};

class IInstanceFilter {
public:
    virtual ~IInstanceFilter() = default;
    virtual InstanceStream Process(const InstanceStream& input) = 0;
};
}
