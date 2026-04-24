#pragma once

#include <imgui.h>

namespace Editor::Themes {

enum class Theme {
    Dark,
    Light,
    Classic,
    PurpleDark,
    PurpleLight,
};

inline void SetDarkPurpleTheme() {
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);

    constexpr ImVec4 purpleBase = ImVec4(0.42f, 0.23f, 0.70f, 1.00f);
    constexpr ImVec4 purpleHover = ImVec4(0.52f, 0.33f, 0.82f, 1.00f);
    constexpr ImVec4 purpleActive = ImVec4(0.60f, 0.40f, 0.90f, 1.00f);
    constexpr ImVec4 purpleMuted = ImVec4(0.27f, 0.24f, 0.33f, 1.00f);

    colors[ImGuiCol_FrameBg] = purpleMuted;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = purpleActive;
    colors[ImGuiCol_TitleBg] = purpleMuted;
    colors[ImGuiCol_TitleBgActive] = purpleMuted;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = purpleMuted;
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    colors[ImGuiCol_CheckMark] = purpleHover;
    colors[ImGuiCol_SliderGrab] = purpleBase;
    colors[ImGuiCol_SliderGrabActive] = purpleActive;
    colors[ImGuiCol_Button] = purpleBase;
    colors[ImGuiCol_ButtonHovered] = purpleHover;
    colors[ImGuiCol_ButtonActive] = purpleActive;

    colors[ImGuiCol_Header] = purpleMuted;
    colors[ImGuiCol_HeaderHovered] = purpleHover;
    colors[ImGuiCol_HeaderActive] = purpleActive;

    colors[ImGuiCol_Separator] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_SeparatorHovered] = purpleHover;
    colors[ImGuiCol_SeparatorActive] = purpleActive;
    colors[ImGuiCol_ResizeGrip] = purpleMuted;
    colors[ImGuiCol_ResizeGripHovered] = purpleHover;
    colors[ImGuiCol_ResizeGripActive] = purpleActive;

    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.15f, 0.40f, 1.00f);
    colors[ImGuiCol_TabSelected] = purpleBase;
    colors[ImGuiCol_TabHovered] = purpleHover;
    colors[ImGuiCol_TabSelectedOverline] = purpleActive;
    colors[ImGuiCol_TabDimmed] = ImVec4(0.15f, 0.10f, 0.20f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.20f, 0.12f, 0.30f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_InputTextCursor] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.42f, 0.23f, 0.70f, 0.70f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = purpleHover;
    colors[ImGuiCol_PlotHistogramHovered] = purpleHover;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextLink] = purpleHover;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.42f, 0.23f, 0.70f, 0.35f);
    colors[ImGuiCol_TreeLines] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    colors[ImGuiCol_DragDropTarget] = purpleHover;
    colors[ImGuiCol_UnsavedMarker] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_NavCursor] = purpleActive;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}

inline void SetLightPurpleTheme() {
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(1.00f, 0.99f, 0.93f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.95f, 0.96f, 0.89f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.99f, 0.94f);

    colors[ImGuiCol_Text] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.55f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);

    constexpr ImVec4 lavenderBase = ImVec4(0.78f, 0.64f, 1.00f, 1.00f);
    constexpr ImVec4 lavenderHover = ImVec4(0.85f, 0.75f, 1.00f, 1.00f);
    constexpr ImVec4 lavenderActive = ImVec4(0.70f, 0.55f, 0.95f, 1.00f);
    constexpr ImVec4 lavenderMuted = ImVec4(0.78f, 0.64f, 1.00f, 0.40f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = lavenderMuted;
    colors[ImGuiCol_FrameBgActive] = lavenderBase;

    colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = lavenderBase;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.85f, 0.85f, 0.88f, 0.51f);

    colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);

    colors[ImGuiCol_CheckMark] = lavenderBase;
    colors[ImGuiCol_SliderGrab] = lavenderBase;
    colors[ImGuiCol_SliderGrabActive] = lavenderActive;
    colors[ImGuiCol_Button] = lavenderBase;
    colors[ImGuiCol_ButtonHovered] = lavenderHover;
    colors[ImGuiCol_ButtonActive] = lavenderActive;

    colors[ImGuiCol_Header] = lavenderMuted;
    colors[ImGuiCol_HeaderHovered] = lavenderHover;
    colors[ImGuiCol_HeaderActive] = lavenderActive;

    colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
    colors[ImGuiCol_SeparatorHovered] = lavenderHover;
    colors[ImGuiCol_SeparatorActive] = lavenderActive;
    colors[ImGuiCol_ResizeGrip] = lavenderMuted;
    colors[ImGuiCol_ResizeGripHovered] = lavenderHover;
    colors[ImGuiCol_ResizeGripActive] = lavenderActive;

    colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.75f, 0.85f, 1.00f);
    colors[ImGuiCol_TabHovered] = lavenderHover;
    colors[ImGuiCol_TabSelected] = lavenderBase;
    colors[ImGuiCol_TabSelectedOverline] = lavenderActive;
    colors[ImGuiCol_TabDimmed] = ImVec4(0.76f, 0.70f, 0.86f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.78f, 0.64f, 1.00f, 1.00f);
    colors[ImGuiCol_TabDimmedSelectedOverline] =
        ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    colors[ImGuiCol_InputTextCursor] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_DockingPreview] = lavenderMuted;
    colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = lavenderHover;
    colors[ImGuiCol_PlotHistogramHovered] = lavenderHover;
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.85f, 0.82f, 0.90f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
    colors[ImGuiCol_TextLink] = lavenderActive;
    colors[ImGuiCol_TextSelectedBg] = lavenderMuted;
    colors[ImGuiCol_TreeLines] = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
    colors[ImGuiCol_DragDropTarget] = lavenderActive;
    colors[ImGuiCol_UnsavedMarker] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavCursor] = lavenderActive;
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);

    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}

inline void SetTheme(Theme theme) {
    switch (theme) {
    case Theme::Dark:
        ImGui::StyleColorsDark();
        break;
    case Theme::Light:
        ImGui::StyleColorsLight();
        break;
    case Theme::Classic:
        ImGui::StyleColorsClassic();
        break;
    case Theme::PurpleDark:
        SetDarkPurpleTheme();
        break;
    case Theme::PurpleLight:
        SetLightPurpleTheme();
        break;
    }
}
} // namespace Editor::Themes
