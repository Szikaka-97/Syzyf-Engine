#pragma once

#include "scatter/modifiers/IModifiers.h"

namespace Scatter {

struct ArraySettings {
public:
    int arraySize = 0;
    glm::vec3 arrayOffset = glm::vec3(0.0f, 1.0f, 0.0f);
public:
    void DrawImGui();
};

class ArrayModifier : public IInstanceModifier {
private:
    ArraySettings settings;
public:
    ArrayModifier(ArraySettings settings);

    InstanceStream Process(const InstanceStream& input);

    void DrawImGui();
};
}
