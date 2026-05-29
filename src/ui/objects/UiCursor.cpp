#include "ui/objects/UiCursor.h"
#include "InputSystem.h"
#include "ui/objects/UiLayout.h"
#include "Graphics.h"
#include "ui/systems/UiLayoutSystem.h"

void UiCursor::Update() {
    InputSystem* input = GetScene()->Input();
    if (!input) return;

    if (input->MouseLocked()) {
        GetNode()->SetEnabled(false);
        return;
    } else {
        GetNode()->SetEnabled(true);
    }

    glm::vec2 mousePosition = input->GetMousePosition();

    if (auto* layout = GetNode()->GetObject<UiLayout>()) {
        glm::vec2 resolution = GetScene()->GetGraphics()->GetScreenResolution();
        float scaleFactor = resolution.y / UiLayoutSystem::VIRTUAL_RESOLUTION.y;

        layout->offset = glm::ivec2((int)mousePosition.x / scaleFactor, (int)mousePosition.y / scaleFactor);
    }
}
