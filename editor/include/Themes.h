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

    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.94f);

    ImVec4 purpleBase = ImVec4(0.50f, 0.25f, 0.80f, 1.00f);
    ImVec4 purpleHover = ImVec4(0.60f, 0.35f, 0.90f, 1.00f);
    ImVec4 purpleActive = ImVec4(0.40f, 0.15f, 0.70f, 1.00f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = purpleActive;

    colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.05f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.30f, 0.15f, 0.50f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);

    colors[ImGuiCol_CheckMark] = purpleHover;
    colors[ImGuiCol_SliderGrab] = purpleBase;
    colors[ImGuiCol_SliderGrabActive] = purpleActive;

    colors[ImGuiCol_Button] = purpleBase;
    colors[ImGuiCol_ButtonHovered] = purpleHover;
    colors[ImGuiCol_ButtonActive] = purpleActive;

    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.20f, 0.60f, 0.80f);
    colors[ImGuiCol_HeaderHovered] = purpleHover;
    colors[ImGuiCol_HeaderActive] = purpleActive;

    colors[ImGuiCol_SeparatorHovered] = purpleHover;
    colors[ImGuiCol_SeparatorActive] = purpleActive;

    colors[ImGuiCol_ResizeGrip] = ImVec4(0.50f, 0.25f, 0.80f, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = purpleHover;
    colors[ImGuiCol_ResizeGripActive] = purpleActive;

    colors[ImGuiCol_Tab] = ImVec4(0.25f, 0.15f, 0.40f, 1.00f);
    colors[ImGuiCol_TabHovered] = purpleHover;
    colors[ImGuiCol_TabActive] = purpleBase;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.10f, 0.20f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.12f, 0.30f, 1.00f);

    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
}

inline void SetLightPurpleTheme() {
    ImGui::StyleColorsLight();

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.96f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.98f, 0.98f, 0.99f, 0.94f);
    colors[ImGuiCol_Text] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

    ImVec4 purpleBase = ImVec4(0.60f, 0.30f, 0.90f, 1.00f);
    ImVec4 purpleHover = ImVec4(0.70f, 0.40f, 1.00f, 1.00f);
    ImVec4 purpleActive = ImVec4(0.50f, 0.20f, 0.80f, 1.00f);
    ImVec4 purpleMuted = ImVec4(0.60f, 0.30f, 0.90f, 0.40f);

    colors[ImGuiCol_FrameBg] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = purpleMuted;
    colors[ImGuiCol_FrameBgActive] = purpleBase;

    colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.88f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = purpleBase;
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.85f, 0.85f, 0.88f, 0.51f);

    colors[ImGuiCol_CheckMark] = purpleBase;
    colors[ImGuiCol_SliderGrab] = purpleBase;
    colors[ImGuiCol_SliderGrabActive] = purpleActive;

    colors[ImGuiCol_Button] = purpleBase;
    colors[ImGuiCol_ButtonHovered] = purpleHover;
    colors[ImGuiCol_ButtonActive] = purpleActive;

    colors[ImGuiCol_Header] = purpleMuted;
    colors[ImGuiCol_HeaderHovered] = purpleHover;
    colors[ImGuiCol_HeaderActive] = purpleActive;

    colors[ImGuiCol_SeparatorHovered] = purpleHover;
    colors[ImGuiCol_SeparatorActive] = purpleActive;

    colors[ImGuiCol_ResizeGrip] = purpleMuted;
    colors[ImGuiCol_ResizeGripHovered] = purpleHover;
    colors[ImGuiCol_ResizeGripActive] = purpleActive;

    colors[ImGuiCol_Tab] = ImVec4(0.80f, 0.75f, 0.85f, 1.00f);
    colors[ImGuiCol_TabHovered] = purpleHover;
    colors[ImGuiCol_TabActive] = purpleBase;
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.90f, 0.88f, 0.92f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.85f, 0.80f, 0.90f, 1.00f);

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
