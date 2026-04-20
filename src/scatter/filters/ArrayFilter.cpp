#include "scatter/filters/ArrayFilter.h"
#include "scatter/Spawner.h"

namespace Scatter {

ArrayFilter::ArrayFilter(ArraySettings settings) : settings(settings) {}

InstanceStream ArrayFilter::Process(const InstanceStream& input) {
    InstanceStream expandedInstances;
    expandedInstances.reserve(input.size() * (1 + this->settings.arraySize));

    for (const auto& instance : input) {
        expandedInstances.push_back(instance);
        for (int i = 0; i < this->settings.arraySize; i++) {
            expandedInstances.push_back({ glm::translate(instance.transform, this->settings.arrayOffset * glm::vec3((1 + i))) });
            }
        }
    return expandedInstances;
}
};

