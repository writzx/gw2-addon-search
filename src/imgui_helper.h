#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGuiExtras {
    inline void BeginDisable(bool allowHover = false) {
        if (allowHover) {
            ImGui::PushItemFlag(ImGuiItemFlags_ReadOnly, true);
        } else {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
    }

    inline void EndDisable() {
        ImGui::PopItemFlag();
        ImGui::PopStyleVar();
    }

    inline void TextWrapped(const std::string &text, const float max_width) {
        if (!text.empty()) {
            ImGui::PushTextWrapPos(max_width);
            ImGui::TextWrapped(text.c_str());
            ImGui::PopTextWrapPos();
        }
    }

    inline void Text(const std::string &text) {
        if (!text.empty()) {
            ImGui::Text(text.c_str());
        }
    }

    inline bool Spinner(const char *label, const float radius, const float thickness, const ImU32 &color) {
        ImGuiWindow *window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;
        const ImGuiStyle &style = g.Style;
        const ImGuiID id = window->GetID(label);

        ImVec2 pos = window->DC.CursorPos;
        ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

        const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
        ImGui::ItemSize(bb, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        // Render
        window->DrawList->PathClear();

        constexpr int num_segments = 30;
        const int start = static_cast<int>(abs(ImSin(g.Time * 1.8f) * (num_segments - 5)));

        const float a_min = IM_PI * 2.0f * static_cast<float>(start) / static_cast<float>(num_segments);
        constexpr float a_max = IM_PI * 2.0f * (static_cast<float>(num_segments) - 3)
                                / static_cast<float>(num_segments);

        const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

        for (int i = 0; i < num_segments; i++) {
            const float a = a_min + (static_cast<float>(i) / static_cast<float>(num_segments)) * (a_max - a_min);
            window->DrawList->PathLineTo(ImVec2(
                centre.x + ImCos(a + g.Time * 8) * radius,
                centre.y + ImSin(a + g.Time * 8) * radius
            ));
        }

        window->DrawList->PathStroke(color, false, thickness);

        return true;
    }
}

inline ImVec2 operator+(const ImVec2 &first, const ImVec2 &second) {
    return {first.x + second.x, first.y + second.y};
}

inline ImVec2 operator-(const ImVec2 &first, const ImVec2 &second) {
    return {first.x - second.x, first.y - second.y};
}

inline ImVec2 operator+(const ImVec2 &first, const float &second) {
    return {first.x + second, first.y + second};
}

inline ImVec2 operator-(const ImVec2 &first, const float &second) {
    return {first.x - second, first.y - second};
}
