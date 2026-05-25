#include "panels/GraphPanel.h"
#include "CameraController.h"
#include "EditorApplication.h"
#include "imgui.h"

#include <Scene.h>

namespace Editor {
void GraphPanel::Draw(Context& context) {
    ImGui::Begin("Graph");

    if (context.selectedScene == nullptr) {
        ImGui::End();
        return;
    }

    SceneNode* root = context.selectedScene->GetRootNode();

    if (root != nullptr) {
        ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f);

        ImGuiTableFlags tableFlags =
            ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
            ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoBordersInBody;

        if (ImGui::BeginTable("Graph Table", 2, tableFlags)) {
            ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch);

            // change so the 30px isnt hardcoded
            ImGui::TableSetupColumn("Visibility",
                                    ImGuiTableColumnFlags_WidthFixed, 30.0f);

            this->DrawGraphNode(context, *root);

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();
    }

    this->DrawContextMenu(context);

    ImGui::End();
}

void GraphPanel::DrawGraphNode(Context& context, SceneNode& node) {
    // Ignore editor camera
    if (node.GetObject<CameraController>()) {
        return;
    }

    ImGui::PushID(node.GetID());

    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    std::string treeHeader = node.GetName();
    if (treeHeader.empty()) {
        treeHeader = std::to_string(node.GetID());
    }

    bool isLeaf = node.GetChildren().empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    if (&node == node.GetScene()->GetRootNode()) {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (context.selectedNode == &node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (isLeaf) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)node.GetID(), flags,
                                      "%s", treeHeader.c_str());

    // Reparenting
    if (ImGui::BeginDragDropSource()) {
        SceneNode* nodePtr = &node;
        ImGui::SetDragDropPayload("GRAPH_SCENE_NODE", &nodePtr,
                                  sizeof(SceneNode*));
        ImGui::Text("Move %s", treeHeader.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("GRAPH_SCENE_NODE")) {
            SceneNode* droppedNode = *(SceneNode**)payload->Data;

            if (droppedNode != droppedNode->GetScene()->GetRootNode() &&
                !node.IsChildOf(droppedNode)) {
                droppedNode->SetParent(&node);
            }
        }
        ImGui::EndDragDropTarget();
    }

    if ((ImGui::IsItemClicked(ImGuiMouseButton_Left) ||
         ImGui::IsItemClicked(ImGuiMouseButton_Right)) &&
        !ImGui::IsItemToggledOpen()) {
        context.selectedNode = &node;
    }

    ImGui::TableNextColumn();

    const bool isEnabled = node.IsEnabled();

    if (!isEnabled) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    }

    if (ImGui::Button(node.EnabledSelf() ? "X" : " ",
                      ImVec2(24, ImGui::GetFrameHeight()))) {
        node.SetEnabled(!node.EnabledSelf());
    }

    if (!isEnabled)
        ImGui::PopStyleColor(3);

    if (nodeOpen) {
        if (!isLeaf) {
            for (SceneNode* child : node.GetChildren()) {
                DrawGraphNode(context, *child);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void GraphPanel::DrawContextMenu(Context& context) {
    bool drawRenamePopup = false;

    if (ImGui::BeginPopupContextWindow("GraphContextMenu",
                                       ImGuiPopupFlags_MouseButtonRight)) {
        bool hasScene = (context.selectedScene != nullptr);

        if (ImGui::MenuItem("Create Node", nullptr, false, hasScene)) {
            if (context.selectedScene != nullptr) {
                SceneNode* parent = context.selectedNode
                                        ? context.selectedNode
                                        : context.selectedScene->GetRootNode();
                context.selectedScene->CreateNode(parent, "New Node");
            }
        }

        if (context.selectedNode != nullptr) {
            if (ImGui::MenuItem("Rename Node")) {
                drawRenamePopup = true;
            }
            if (ImGui::MenuItem("Delete Node")) {
                delete context.selectedNode;

                context.selectedNode = nullptr;
            }
        }
        ImGui::EndPopup();
    }

    // Deselect node if empty space is pressed
    if (ImGui::IsWindowHovered() &&
        (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
         ImGui::IsMouseClicked(ImGuiMouseButton_Right))) {
        if (!ImGui::IsAnyItemHovered()) {
            context.selectedNode = nullptr;
        }
    }

    if (drawRenamePopup) {
        ImGui::OpenPopup("RenamePopup");
    }

    if (ImGui::BeginPopup("RenamePopup")) {
        static char nameBuffer[256] = "";

        if (context.selectedNode != nullptr) {
            if (ImGui::IsWindowAppearing()) {
                strncpy(nameBuffer, context.selectedNode->GetName().c_str(),
                        sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
                ImGui::SetKeyboardFocusHere();
            }

            bool applyRename = false;
            if (ImGui::InputText("##NewNodeName", nameBuffer,
                                 sizeof(nameBuffer),
                                 ImGuiInputTextFlags_EnterReturnsTrue)) {
                applyRename = true;
            }

            ImGui::SameLine();
            if (ImGui::Button("Save")) {
                applyRename = true;
            }

            if (applyRename) {
                context.selectedNode->SetName(nameBuffer);
                ImGui::CloseCurrentPopup();
            }
        } else {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}
} // namespace Editor
