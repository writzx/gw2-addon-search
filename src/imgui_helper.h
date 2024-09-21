#pragma once
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGuiHelper {
	void BeginDisable() {
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
	}

	void EndDisable() {
		ImGui::PopItemFlag();
		ImGui::PopStyleVar();
	}

	void TextWrapped(std::string text, float max_width) {
		if (text.size() > 0) {
			ImGui::PushTextWrapPos(max_width);
			ImGui::TextWrapped(text.c_str());
			ImGui::PopTextWrapPos();
		}
	}

	void Text(std::string text) {
		if (text.size() > 0) {
			ImGui::Text(text.c_str());
		}
	}
}