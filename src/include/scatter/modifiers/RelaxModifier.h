#pragma once

#include "scatter/modifiers/IModifiers.h"

namespace Scatter {

struct RelaxSettings {
public:
    float minDistance = 2.0f;
    // max number of attempts it will try to place the object
    //  before giving up
    int maxAttempts = 30;
public:
    void DrawImGui();
};

class RelaxModifier : public IPointModifier {
private:
    RelaxSettings settings;
public:
    RelaxModifier(RelaxSettings settings);

    PointStream Process(const PointStream& input);
};
}
