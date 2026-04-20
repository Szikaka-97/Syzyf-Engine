#pragma once

#include "scatter/filters/IFilters.h"

namespace Scatter {

struct RelaxSettings {
    float minDistance = 2.0f;
    // max number of attempts it will try to place the object
    //  before giving up
    int maxAttempts = 30; 
};

class RelaxFilter : public IPointFilter {
private:
    RelaxSettings settings;
public:
    RelaxFilter(RelaxSettings settings);

    PointStream Process(const PointStream& input);
};
}
