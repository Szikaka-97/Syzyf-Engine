#include "panels/SystemsDebugPanel.h"
#include "Application.h"
#include "Graphics.h"

#include <Scene.h>
#include <imgui.h>
#include <physics/System.h>

namespace Editor {
void SystemsDebugPanel::Draw(Context& context) {
    ImGui::Begin("Systems");

    if (context.selectedScene) {
        context.selectedScene->DrawImGui();
    }

    ImGui::End();
}
} // namespace Editor
