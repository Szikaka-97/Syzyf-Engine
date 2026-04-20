#include "scatter/filters/RelaxFilter.h"

namespace Scatter {

RelaxFilter::RelaxFilter(RelaxSettings settings) : settings(settings) {}

PointStream RelaxFilter::Process(const PointStream& input) {
    PointStream validPositions;
    validPositions.reserve(input.size());

    for (const glm::vec3& position : input) {
        bool isOverlapping = false;
        for (const glm::vec3& exisitingPosition : validPositions) {
            if (glm::distance(position, exisitingPosition) < this->settings.minDistance) {
            isOverlapping = true;
            break;
            }
        }

        if (!isOverlapping) {
            validPositions.push_back(position);
        }
    }
    return validPositions;
}
};
