#pragma once

#include <glm/glm.hpp>

namespace Scatter {

struct InstanceData;

using PointStream = std::vector<glm::vec3>;
using InstanceStream = std::vector<InstanceData>;

class IPointModifier {
public:
    virtual ~IPointModifier() = default;
    virtual PointStream Process(const PointStream& input) = 0;
};

class IInstanceModifier {
public:
    virtual ~IInstanceModifier() = default;
    virtual InstanceStream Process(const InstanceStream& input) = 0;
};

class IPointToInstanceModifier {
public:
    virtual ~IPointToInstanceModifier() = default;
    virtual InstanceStream Process(const PointStream& input) = 0;
};
}
