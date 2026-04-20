#pragma once

#include "scatter/filters/IFilters.h"

namespace Scatter {

struct ArraySettings {
    int arraySize = 0;
    glm::vec3 arrayOffset = glm::vec3(0.0f, 1.0f, 0.0f);
};

class ArrayFilter : public IInstanceFilter {
private:
    ArraySettings settings;
public:
    ArrayFilter(ArraySettings settings);

    InstanceStream Process(const InstanceStream& input);
};
}
