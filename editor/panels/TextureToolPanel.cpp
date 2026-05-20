#include "panels/TextureToolPanel.h"

#include <imgui.h>

namespace Editor {
void TextureToolPanel::Draw(Context& context) {
    ImGui::Begin("Texture Tool");
    ImGui::Text("Pooga");
    ImGui::End();
}
} // namespace Editor
