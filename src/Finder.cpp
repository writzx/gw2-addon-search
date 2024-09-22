#include "Finder.h"
#include <imgui/imgui.h>
#include <thread>

#include "imgui_helper.h"
#include <resource.h>

void* Finder::LoadRemoteTexture(const char* identifier, const char* url) const {
	if (this->load_remote_texture != nullptr) {
		return this->load_remote_texture(identifier, url);
	}

	return nullptr;
}

void* Finder::LoadResourceTexture(const char* identifier, int resourceId) const {
	if (this->load_resource_texture != nullptr) {
		return this->load_resource_texture(identifier, resourceId);
	}

	return nullptr;
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
	ImVec2 savedCursor;

	ImGuiIO& io = ImGui::GetIO();

	if (this->store != nullptr) {
		this->tick();
	}

	ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, 300.0f), ImVec2(FLT_MAX, FLT_MAX));

	auto winSize = ImGui::GetWindowSize();
	auto numGridCols = std::floor(winSize.x / GRID_ITEM_SIZE) - 1;

	if (ImGui::Begin("Finder")) {
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (this->store != nullptr && this->client != nullptr && this->store->refreshing) {
			ImGuiHelper::BeginDisable();
		}

		auto apiButton = ImGui::Button("API");

		if (apiButton) {
			this->state->api_window = true;
			ImGui::OpenPopup("API Key");
		}

		if (this->store != nullptr && this->client != nullptr && this->store->refreshing) {
			ImGuiHelper::EndDisable();
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

			auto doSearch = ImGui::InputText("##search_input", this->state->query, IM_ARRAYSIZE(this->state->query), ImGuiInputTextFlags_EnterReturnsTrue);

			ImGui::SameLine();

			doSearch = ImGui::Button("Search") || doSearch;

			if (!this->state->can_search) {
				ImGuiHelper::EndDisable();
			}

			if (doSearch) {
				helper::str_trim(this->state->query);
				this->state->items.clear();

				items.clear();
				this->store->search(this->state->query, items);

				for (auto& item : items) {
					if (!this->state->items.contains(item.endpoint_path)) {
						std::vector<Item> ep_items;
						this->state->items[item.endpoint_path] = ep_items;
					}

					this->state->items[item.endpoint_path].push_back(item);
				}
			}

			for (const auto& [ep, results] : this->state->items) {
				if (results.size() <= 0) {
					continue;
				}

				if (ImGui::CollapsingHeader(std::format("{}", results[0].endpoint).c_str()), ImGuiTreeNodeFlags_DefaultOpen) {
					if (ImGui::BeginTable(std::format("Table: {}", ep).c_str(), numGridCols)) {
						for (int col = 0; col < numGridCols; col++) {
							ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, GRID_ITEM_SIZE);
						}

						for (auto& result : results) {
							void* texture = this->LoadRemoteTexture(std::format("ITEM_TEX_{}", result.id).c_str(), result.icon.c_str());

							ImGui::TableNextColumn();
							ImGui::Image(texture, ImVec2(GRID_ITEM_SIZE, GRID_ITEM_SIZE));

							auto minPoint = ImGui::GetItemRectMin();
							auto maxPoint = ImGui::GetItemRectMax();

							std::string rarity = result.rarity;

							bool hovered = ImGui::IsItemHovered();
							auto& rarity_color = BORDER_COLORS.at(rarity);
							auto& rarity_color_hover = BORDER_COLORS_HOVER.at(rarity);

							drawList->AddRect(minPoint, maxPoint, ImGui::ColorConvertFloat4ToU32(hovered ? rarity_color_hover : rarity_color), 0.0f, 0, BORDER_WIDTH);

							std::string count = std::format("{}", result.count > 1 ? result.count : result.charges > 1 ? result.charges : 0);

							if (count != "0") {
								savedCursor = ImGui::GetCursorPos();

								auto textSize = ImGui::CalcTextSize(count.c_str());

								ImGui::SetCursorScreenPos(ImVec2(maxPoint.x - textSize.x - 3.0f, maxPoint.y - textSize.y - 2.0f));

								ImGui::Text(count.c_str());

								ImGui::SetCursorPos(savedCursor);
							}

							if (hovered) {
								ImGui::SetNextWindowSizeConstraints(ImVec2(FLT_MIN, FLT_MIN), ImVec2(300.0f, FLT_MAX));

								ImGui::BeginTooltip();

								ImDrawList* tooltipDrawList = ImGui::GetWindowDrawList();

								ImGui::Image(texture, ImVec2(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE));

								minPoint = ImGui::GetItemRectMin();
								maxPoint = ImGui::GetItemRectMax();

								tooltipDrawList->AddRect(minPoint, maxPoint, IM_COL32(255, 255, 255, 255), 0.0f, 0, BORDER_WIDTH);
								ImGui::SameLine();

								savedCursor = ImGui::GetCursorPos();

								auto titleSize = ImGui::CalcTextSize(result.display_name().c_str());

								ImGui::SetCursorPosY(savedCursor.y + (maxPoint.y - minPoint.y - titleSize.y) / 2);

								ImGui::TextColored(rarity_color, result.display_name().c_str());

								ImGuiHelper::TextWrapped(result.clean_description(), 300.0f);

								ImGui::Dummy(ImVec2(0.0f, 8.0f));

								ImGuiHelper::Text(result.display_type());
								ImGuiHelper::Text(result.required_level());
								ImGuiHelper::Text(result.display_binding());

								if (result.vendor_value > 0) {
									ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

									float textHeight = ImGui::CalcTextSize("m").y;

									int copper = result.vendor_value * result.count;
									int gold = copper / 10000;

									copper %= 10000;
									int silver = copper / 100;

									copper %= 100;

									savedCursor = ImGui::GetCursorPos();

									if (gold > 0) {
										ImGui::TextColored(COLOR_CURRENCY_GOLD, std::format("{}", gold).c_str());
										ImGui::SameLine();

										void* gold_texture = this->LoadResourceTexture("gold", GOLD_PNG);

										ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
										ImGui::Image(gold_texture, ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

										ImGui::SameLine();
										ImGui::SetCursorPosY(savedCursor.y);

										ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
										ImGui::Dummy(ImVec2(1.0f, 0.f));
										ImGui::PopStyleVar();

										ImGui::SameLine();
									}

									if (silver > 0 || gold > 0) {
										ImGui::TextColored(COLOR_CURRENCY_SILVER, std::format("{}", silver).c_str());
										ImGui::SameLine();

										void* silver_texture = this->LoadResourceTexture("silver", SILVER_PNG);

										ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
										ImGui::Image(silver_texture, ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

										ImGui::SameLine();
										ImGui::SetCursorPosY(savedCursor.y);

										ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
										ImGui::Dummy(ImVec2(1.0f, 0.f));
										ImGui::PopStyleVar();

										ImGui::SameLine();
									}

									if (copper > 0 || silver > 0 || gold > 0) {
										ImGui::TextColored(COLOR_CURRENCY_COPPER, std::format("{}", copper).c_str());
										ImGui::SameLine();

										void* copper_texture = this->LoadResourceTexture("copper", COPPER_PNG);

										ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
										ImGui::Image(copper_texture, ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

										ImGui::SameLine();

										ImGui::SetCursorPosY(savedCursor.y);

										ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
										ImGui::Dummy(ImVec2(1.0f, 0.f));
										ImGui::PopStyleVar();
									}

									ImGui::PopStyleVar();
								}

								ImGui::EndTooltip();
							}
						}
						ImGui::EndTable();
					}
				}
			}
		}

		ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, FLT_MIN), ImVec2(400.0f, FLT_MAX));

		if (ImGui::BeginPopupModal("API Key", &this->state->api_window, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar)) {
			ImGui::Text(std::format("Account ID: {0}", this->id).c_str());

			auto save = ImGui::InputText("##api_key", this->state->key_buffer, IM_ARRAYSIZE(this->state->key_buffer), ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue);

			save = ImGui::Button("Save") || save;

			if (save) {
				helper::str_trim(this->state->key_buffer);

				this->config->set_api_key(this->id, this->state->key_buffer);
				this->config->save();

				this->client->update_token(this->config->get_api_key(this->id));

				ImGui::CloseCurrentPopup();
			}

			ImGui::SameLine();
			auto cancel = ImGui::Button("Cancel");

			if (cancel) {
				// restore the old api key
				strcpy_s(this->state->key_buffer, this->config->get_api_key(this->id).c_str());

				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	ImGui::End();
}

void Finder::SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader) {
	this->load_remote_texture = texture_loader;
}

void Finder::SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader) {
	this->load_resource_texture = texture_loader;
}
