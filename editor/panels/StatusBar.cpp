#include "panels/StatusBar.h"

#include "EditorApplication.h"
#include "imgui.h"

namespace Editor {
void StatusBar::Draw() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    float statusBarHeight =
        ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;

    ImVec2 windowPosition =
        ImVec2(viewport->WorkPos.x,
               viewport->WorkPos.y + viewport->WorkSize.y - statusBarHeight);
    ImVec2 windowSize = ImVec2(viewport->WorkSize.x, statusBarHeight);

    ImGui::SetNextWindowPos(windowPosition);
    ImGui::SetNextWindowSize(windowSize);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("##StatusBar", nullptr, windowFlags)) {
        ImGui::Text("LMB: Select");

        ImGui::SameLine(0.0f, 20.0f);

        ImGui::Text("LCtrl + LMB: Select Root");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}
} // namespace Editor
