#include "Finder.h"
#include <imgui/imgui.h>
#include <thread>

#include "imgui_helper.h"

void Finder::LoadTexture(const char* identifier, const char* url, ItemTexture*& out_texture) const {
	if (this->load_texture != nullptr) {
		this->load_texture(identifier, url, out_texture);
	}
}

void Finder::InitImGui(void* ctxt, void* imgui_malloc, void* imgui_free) {
	ImGui::SetCurrentContext((ImGuiContext*) ctxt);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))imgui_malloc, (void(*)(void*, void*))imgui_free);
}

void Finder::refresh_store() noexcept {
	std::jthread(
		[&] {
			try {
				store->refresh();
			} catch (...) {
				// ignore exception
			}
		}
	).detach();
}

void Finder::tick() noexcept {
	auto tick = chrono::now();

	auto delta_time = this->last_tick.has_value() ? (tick - this->last_tick.value()) : 5min;

	// ignore update before ui tick interval
	if (delta_time <= UI_TICK_INTERVAL) {
		return;
	}

	this->last_tick = tick;

	this->state->needs_refresh = this->store->should_auto_refresh();
	this->state->can_manual_refresh = this->store->can_manual_refresh();
	this->state->can_search = this->store->can_search();

	if (this->state->needs_refresh) {
		//refresh_store();
	}
}

void Finder::Render() {
	ImGuiIO& io = ImGui::GetIO();

	if (this->store != nullptr) {
		this->tick();
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 300.0f), ImVec2(FLT_MAX, FLT_MAX));

	auto winSize = ImGui::GetWindowSize();
	auto numGridCols = std::floor(winSize.x / GRID_ITEM_SIZE) - 1;

	if (ImGui::Begin("Finder")) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		auto apiButton = ImGui::Button("API");

		if (apiButton) {
			this->state->api_window = true;
			ImGui::OpenPopup("API Key");
		}

		ImGui::SameLine();

		if (this->store == nullptr || this->client == nullptr) {
			ImGui::Text("please add an api key");
		} else {

#pragma region refresh button
			auto refreshBtn = false;

			bool can_refresh = this->state->can_manual_refresh && !this->store->refreshing;

			if (!can_refresh) {
				ImGuiHelper::BeginDisable();
			}

			refreshBtn = ImGui::Button("Refresh");

			if (!can_refresh) {
				ImGuiHelper::EndDisable();
			}

			if (refreshBtn && can_refresh) {
				refresh_store();
			}
#pragma endregion

			ImGui::SameLine();

			auto last_updated = this->store->last_updated();
			ImGui::TextWrapped(last_updated.has_value() ? helper::datetime_tostring(last_updated.value()).c_str() : "never");

			ImGui::TextWrapped(this->store->status.c_str());

			if (!this->state->can_search) {
				ImGuiHelper::BeginDisable();
			}

			auto doSearch = false;

			if (ImGui::InputText("##search_input", this->state->query, IM_ARRAYSIZE(this->state->query), ImGuiInputTextFlags_EnterReturnsTrue)) {
				doSearch = true;
			}
			ImGui::SameLine();
			auto searchBtn = ImGui::Button("Search");

			if (!this->state->can_search) {
				ImGuiHelper::EndDisable();
			}

			if (searchBtn) {
				doSearch = true;
			}

			if (doSearch) {
				helper::str_trim(this->state->query);
				this->state->items.clear();

				items.clear();
				this->store->search(this->state->query, items);

				for (auto& item : items) {
					if (!this->state->items.contains(item.endpoint)) {
						std::vector<FinderItem*> ep_items;
						this->state->items[item.endpoint] = ep_items;
					}

					FinderItem* newItem = new FinderItem();

					newItem->item = &item;

					newItem->thumb = new ItemTexture();

					this->LoadTexture(std::format("ITEM_TEX_{}", item.id).c_str(), item.icon.c_str(), newItem->thumb);

					this->state->items[item.endpoint].push_back(newItem);
				}
			}

			for (const auto& [ep, results] : this->state->items) {
				if (ImGui::CollapsingHeader(std::format("{}", ep).c_str()), ImGuiTreeNodeFlags_DefaultOpen) {
					if (ImGui::BeginTable(std::format("Table: {}", ep).c_str(), numGridCols)) {
						for (int col = 0; col < numGridCols; col++) {
							ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GRID_ITEM_SIZE);
						}

						for (auto& result : results) {
							this->LoadTexture(std::format("ITEM_TEX_{}", result->item->id).c_str(), result->item->icon.c_str(), result->thumb);

							ImGui::TableNextColumn();
							ImGui::Image(result->thumb->shader, ImVec2(GRID_ITEM_SIZE, GRID_ITEM_SIZE));

							auto minPoint = ImGui::GetItemRectMin();
							auto maxPoint = ImGui::GetItemRectMax();

							std::string rarity = result->item->rarity;

							bool hovered = ImGui::IsItemHovered();
							auto& rarity_color = BORDER_COLORS.at(rarity);
							auto& rarity_color_hover = BORDER_COLORS_HOVER.at(rarity);

							drawList->AddRect(minPoint, maxPoint, ImGui::ColorConvertFloat4ToU32(hovered ? rarity_color_hover : rarity_color), 0.0f, 0, BORDER_WIDTH);

							if (hovered) {
								ImGui::SetNextWindowSizeConstraints(ImVec2(FLT_MIN, FLT_MIN), ImVec2(300.0f, FLT_MAX));

								ImGui::BeginTooltip();

								ImDrawList* tooltipDrawList = ImGui::GetWindowDrawList();

								ImGui::Image(result->thumb->shader, ImVec2(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE));

								minPoint = ImGui::GetItemRectMin();
								maxPoint = ImGui::GetItemRectMax();

								tooltipDrawList->AddRect(minPoint, maxPoint, IM_COL32(255, 255, 255, 255), 0.0f, 0, BORDER_WIDTH);

								ImGui::SameLine();
								ImGui::SetCursorPosY(16.0f);
								ImGui::TextColored(rarity_color, result->item->display_name().c_str());

								ImGuiHelper::TextWrapped(result->item->clean_description(), 300.0f);

								ImGui::Dummy(ImVec2(0.0f, 8.0f));

								ImGuiHelper::Text(result->item->display_type());
								ImGuiHelper::Text(result->item->required_level());
								ImGuiHelper::Text(result->item->display_binding());

								ImGui::EndTooltip();
							}
						}
						ImGui::EndTable();
					}
				}
			}

			ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, FLT_MIN), ImVec2(400.0f, FLT_MAX));

			if (ImGui::BeginPopupModal("API Key", &this->state->api_window, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
				ImGui::Text(std::format("Account ID: {0}", this->id).c_str());
				ImGui::InputText("##api_key", this->state->key_buffer, IM_ARRAYSIZE(this->state->key_buffer), ImGuiInputTextFlags_Password);

				auto save = ImGui::Button("Save");
				ImGui::SameLine();
				auto cancel = ImGui::Button("Cancel");

				if (cancel) {
					this->client->update_token(this->config->get_api_key(this->id));

					ImGui::CloseCurrentPopup();
				}

				if (save) {
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}
		}
	}
	ImGui::End();
}

void Finder::SetTextureLoader(LOAD_TEXTURE texture_loader) {
	this->load_texture = texture_loader;
}
