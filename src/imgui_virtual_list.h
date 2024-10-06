#pragma once
#include <functional>
#include <imgui/imgui.h>

typedef float (*MEASURE_ITEM)(int index);

typedef float (*RENDER_ITEM)(int index);

namespace ImGui {
    inline void VirtualListV(
        const char *label,
        const unsigned long count,
        const std::function<float(int)> &measure_item,
        const std::function<float(int)> &render_item,
        const float gap = 0.0f,
        const ImVec2 &size = ImVec2(0, 0)
    ) {
        if (count == 0) {
            return;
        }
        const auto style = GetStyle();

        PushStyleVar(ImGuiStyleVar_ChildRounding, style.FrameRounding);
        PushStyleVar(ImGuiStyleVar_WindowPadding, style.FramePadding);

        const auto outer_window = BeginChild(
            label,
            size,
            false,
            ImGuiWindowFlags_NoNav
        );

        PopStyleVar(2);

        if (outer_window) {
            const auto window_size = GetContentRegionAvail();
            const float scroll_y = GetScrollY();
            float total_height = 0;

            float render_top = 0;
            int first = -1, last = static_cast<int>(count) - 1;

            for (auto i = 0; i < count; i++) {
                const auto item_height = measure_item(i) + gap;
                const auto final_height = total_height + item_height;

                if (final_height >= scroll_y && first == -1) {
                    first = i;
                    render_top = total_height;
                }

                if (final_height >= scroll_y + window_size.y && last == count - 1) {
                    last = i;
                }

                total_height = final_height;
            }

            // there are no more items, remove the last gap that was added for it!
            total_height -= gap;

            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

            const auto inner_window = BeginChild(
                "##virtual_inner_window",
                ImVec2(window_size.x, total_height),
                false,
                ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration
                | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollWithMouse
            );

            PopStyleVar();

            float y = render_top;

            if (inner_window) {
                for (int i = first; i <= last; i++) {
                    SetCursorPos(ImVec2(0.0f, y));
                    y += render_item(i) + gap;
                }
            }

            EndChild();
        }

        EndChild();
    }
}
