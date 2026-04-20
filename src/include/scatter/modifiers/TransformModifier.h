#pragma once

#include "scatter/modifiers/IModifiers.h"

namespace Scatter {

struct TransformSettings {
public:
    float minScale = 1.0f;
    float maxScale = 1.0f;

    glm::vec3 minRotation = { 0.0f, 0.0f, 0.0f };
    glm::vec3 maxRotation = { 0.0f, 0.0f, 0.0f };
public:
    void DrawImGui();
};

class TransformModifier : public IPointToInstanceModifier {
private:
    TransformSettings settings;
public:
    TransformModifier(TransformSettings settings);

    InstanceStream Process(const PointStream& input);
};
}
