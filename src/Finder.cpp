#include "Finder.h"
#include <imgui/imgui.h>
#include <thread>

#include "imgui_helper.h"
#include <resource.h>

void *Finder::LoadRemoteTexture(const char *identifier, const char *url) const {
    if (this->load_remote_texture != nullptr) {
        return this->load_remote_texture(identifier, url);
    }

    return nullptr;
}

void *Finder::LoadResourceTexture(const char *identifier, const int resourceId) const {
    if (this->load_resource_texture != nullptr) {
        return this->load_resource_texture(identifier, resourceId);
    }

    return nullptr;
}

void Finder::Show() {
    this->is_shown = true;
}

void Finder::Hide() {
    this->is_shown = false;
}

void Finder::Toggle() {
    if (this->is_shown) {
        this->Hide();
    } else {
        this->Show();
    }
}

void Finder::InitImGui(void *ctxt, void *imgui_malloc, void *imgui_free) {
    ImGui::SetCurrentContext(static_cast<ImGuiContext *>(ctxt));
    ImGui::SetAllocatorFunctions(
        static_cast<void* (*)(size_t, void *)>(imgui_malloc),
        static_cast<void(*)(void *, void *)>(imgui_free)
    );
}

void Finder::refresh_store() const noexcept {
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

    // ignore update before ui tick interval
    if (
        const auto delta_time = this->last_tick.has_value() ? tick - this->last_tick.value() : 5min;
        delta_time <= UI_TICK_INTERVAL
    ) {
        return;
    }

    this->last_tick = tick;

    this->state->needs_refresh = this->store->should_auto_refresh();
    this->state->can_manual_refresh = this->store->can_manual_refresh();
    this->state->can_search = this->store->can_search();

    if (this->state->needs_refresh) {
        // refresh_store();
    }
}

void Finder::Render() {
    ImGuiIO &io = ImGui::GetIO();

    ImVec2 savedCursor, minPoint, maxPoint, itemSize;
    bool hovered;
    auto &style = ImGui::GetStyle();
    auto frame_padding = style.FramePadding;
    auto window_padding = style.WindowPadding;
    auto item_spacing = style.ItemSpacing;
    auto window_size = ImGui::GetContentRegionAvail();

    auto grid_item_size_with_spacing = GRID_ITEM_SIZE + item_spacing.y;

    if (this->store != nullptr) {
        this->tick();
    }

    if (!this->is_shown) {
        return;
    }

    // account for item spacing when calculating number of columns
    int grid_col_count = static_cast<int>((window_size.x - item_spacing.x) / (GRID_ITEM_SIZE + item_spacing.x)) - 1;

    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 300.0f), ImVec2(FLT_MAX, FLT_MAX));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    if (ImGui::Begin("Finder", &this->is_shown)) {
        const float search_bar_height = REFRESH_BUTTON_SIZE + window_padding.y * 2;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window_padding);
        auto header_area = ImGui::BeginChild(
            "##header_area",
            ImVec2(0, search_bar_height),
            false,
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysUseWindowPadding |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );
        ImGui::PopStyleVar();

        if (header_area) {
            if (this->store != nullptr && this->client != nullptr && this->store->refreshing) {
                ImGuiExtras::BeginDisable();
            }

            if (ImGui::Button("API", ImVec2(0, REFRESH_BUTTON_SIZE))) {
                this->state->api_window = true;
                ImGui::OpenPopup("API Key");
            }

            if (this->store != nullptr && this->client != nullptr && this->store->refreshing) {
                ImGuiExtras::EndDisable();
            }

            ImGui::SameLine();

            if (this->store == nullptr || this->client == nullptr) {
                ImGui::Text("please add an api key");
            } else {
#pragma region refresh button
                auto refreshBtn = false;

                const bool can_refresh = this->state->can_manual_refresh && !this->store->refreshing;

                if (!can_refresh) {
                    ImGuiExtras::BeginDisable(true);
                }

                savedCursor = ImGui::GetCursorScreenPos();
                refreshBtn = ImGui::Button("##refresh", ImVec2(REFRESH_BUTTON_SIZE, REFRESH_BUTTON_SIZE));

                hovered = ImGui::IsItemHovered();

                const auto spinnerLocation = savedCursor - frame_padding - REFRESH_SPINNER_RADIUS + REFRESH_BUTTON_SIZE
                                             * 0.5;

                ImGui::SetCursorScreenPos(spinnerLocation);

                if (this->store->refreshing) {
                    ImGui::SetCursorScreenPos(ImVec2(spinnerLocation.x + frame_padding.x, spinnerLocation.y));
                    ImGuiExtras::Spinner(
                        "##spinner",
                        REFRESH_SPINNER_RADIUS,
                        REFRESH_SPINNER_THICKNESS,
                        IM_COL32(220, 220, 220, 255)
                    );
                } else {
                    void *refresh_texture = this->LoadResourceTexture("refresh", REFRESH_PNG);

                    ImGui::Image(refresh_texture, ImVec2(REFRESH_BUTTON_SIZE, REFRESH_BUTTON_SIZE));
                }

                if (!can_refresh) {
                    ImGuiExtras::EndDisable();
                }

                if (refreshBtn && can_refresh) {
                    refresh_store();
                }
#pragma endregion

                if (hovered) {
                    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, FLT_MIN), ImVec2(300.0f, FLT_MAX));

                    ImGui::BeginTooltip();

                    const auto last_updated = this->store->last_updated();
                    ImGui::TextWrapped(
                        last_updated.has_value()
                            ? helper::datetime_tostring(last_updated.value()).c_str()
                            : "never"
                    );

                    ImGui::TextWrapped(this->store->status.c_str());

                    ImGui::EndTooltip();
                }

                ImGui::SameLine();

                if (!this->state->can_search) {
                    ImGuiExtras::BeginDisable();
                }

                ImGui::SetCursorScreenPos(
                    ImVec2(spinnerLocation.x + REFRESH_BUTTON_SIZE + item_spacing.x, spinnerLocation.y));
                auto doSearch = ImGui::InputText(
                    "##search_input",
                    this->state->query,
                    IM_ARRAYSIZE(this->state->query),
                    ImGuiInputTextFlags_EnterReturnsTrue
                );

                ImGui::SameLine();

                doSearch = ImGui::Button("Search", ImVec2(0, REFRESH_BUTTON_SIZE)) || doSearch;

                if (!this->state->can_search) {
                    ImGuiExtras::EndDisable();
                }

                if (doSearch) {
                    helper::str_trim(this->state->query);
                    if (strlen(this->state->query) > 2) {
                        this->state->item_sections.clear();

                        items.clear();
                        // todo move this to a thread
                        this->store->search(this->state->query, items);

                        for (auto &item: items) {
                            if (!this->state->item_sections.contains(item.endpoint_path)) {
                                this->state->item_sections[item.endpoint_path] = {
                                    .label = item.endpoint,
                                    .items = {},
                                };
                            }
                            this->state->item_sections[item.endpoint_path].items.push_back(&item);
                        }

                        float top = 0;

                        for (auto &[ep, section]: this->state->item_sections) {
                            section.top = top;
                            section.items_area_height =
                                    std::ceil(
                                        static_cast<float>(section.items.size())
                                        / static_cast<float>(grid_col_count)
                                    ) * grid_item_size_with_spacing + item_spacing.y;

                            // add header height
                            top += ImGui::GetTextLineHeightWithSpacing(); // todo maybe customize this anyway
                            // add items area height
                            top += section.items_area_height;
                        }
                    }
                }
            }
        }
        ImGui::EndChild();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window_padding);
        auto search_result_area = ImGui::BeginChild(
            "##search_result_area",
            ImVec2(0, 0),
            false,
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysUseWindowPadding
        );
        ImGui::PopStyleVar();

        if (search_result_area) {
            float scroll_y = ImGui::GetScrollY();

            auto ep_header_height = ImGui::GetTextLineHeightWithSpacing();

            auto upper_dummy = false;
            auto lower_dummy = false;

            // search result area drawlist
            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            for (const auto &[ep, section]: this->state->item_sections) {
                // ignore empty endpoints completely
                // since we don't show any items from these
                if (section.items.empty()) {
                    continue;
                }

                if (section.top > scroll_y + window_size.y) {
                    // section is already below the visible area
                    // don't render anymore
                    break;
                }

                // only draw header if it is at least partially in the visible area
                if (section.top + ep_header_height > scroll_y) {
                    if (!upper_dummy) {
                        // place upper dummy
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                        ImGui::Dummy(ImVec2(window_size.x, section.top));
                        ImGui::PopStyleVar();

                        upper_dummy = true;
                    }

                    // render the header
                    ImGui::CollapsingHeader(section.label.c_str(), ImGuiTreeNodeFlags_Leaf);
                }

                auto items_area_top = section.top + ep_header_height;
                auto items_area_bottom = section.top + ep_header_height + section.items_area_height;

                auto hidden_top = scroll_y - items_area_top;
                auto hidden_bottom = items_area_bottom - scroll_y + window_size.y;

                int first_visible_row = std::floor((
                                                       std::max(items_area_top, scroll_y) - items_area_top
                                                   ) / grid_item_size_with_spacing);
                int last_visible_row = std::ceil((
                                                     std::min(items_area_bottom, scroll_y + window_size.y) -
                                                     items_area_top
                                                 ) / grid_item_size_with_spacing);

                auto table_top = items_area_top
                                 + static_cast<float>(first_visible_row - 1)
                                 * grid_item_size_with_spacing;

                if (!upper_dummy && hidden_top >= 0) {
                    // place upper dummy
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::Dummy(ImVec2(window_size.x, table_top));
                    ImGui::PopStyleVar();

                    upper_dummy = true;
                }

                ImGui::PushItemWidth(window_size.x);
                if (ImGui::BeginTable(std::format("##table_{}", ep).c_str(), grid_col_count)) {
                    for (int col = 0; col < grid_col_count; col++) {
                        ImGui::TableSetupColumn(
                            std::format("##col_{}_{}", ep, col).c_str(),
                            ImGuiTableColumnFlags_WidthFixed,
                            GRID_ITEM_SIZE
                        );
                    }

                    for (
                        int i = std::max((first_visible_row - 1) * grid_col_count, 0);
                        i <= std::min(last_visible_row * grid_col_count, static_cast<int>(section.items.size()) - 1);
                        i++
                    ) {
                        auto result = section.items[i];
                        void *texture = this->LoadRemoteTexture(
                            std::format("ITEM_TEX_{}", result->id).c_str(),
                            result->icon.c_str()
                        );

                        ImGui::TableNextColumn();
                        ImGui::Image(texture, ImVec2(GRID_ITEM_SIZE, GRID_ITEM_SIZE));

                        minPoint = ImGui::GetItemRectMin();
                        maxPoint = ImGui::GetItemRectMax();

                        std::string rarity = result->rarity;

                        hovered = ImGui::IsItemHovered();
                        auto &rarity_color = BORDER_COLORS.at(rarity);
                        auto &rarity_color_hover = BORDER_COLORS_HOVER.at(rarity);

                        draw_list->AddRect(
                            minPoint,
                            maxPoint,
                            ImGui::ColorConvertFloat4ToU32(
                                hovered ? rarity_color_hover : rarity_color),
                            0.0f,
                            0,
                            BORDER_WIDTH
                        );

                        auto count = result->count_or_charges();

                        if (!count.empty()) {
                            savedCursor = ImGui::GetCursorPos();

                            const auto textSize = ImGui::CalcTextSize(count.c_str());

                            ImGui::SetCursorScreenPos(
                                ImVec2(maxPoint.x - textSize.x - 3.0f, maxPoint.y - textSize.y - 2.0f));

                            ImGui::Text(count.c_str());

                            ImGui::SetCursorPos(savedCursor);
                        }
#pragma region item tooltip
                        if (hovered) {
                            ImGui::SetNextWindowSizeConstraints(
                                ImVec2(FLT_MIN, FLT_MIN),
                                ImVec2(300.0f, FLT_MAX));

                            ImGui::BeginTooltip();

                            ImDrawList *tooltipDrawList = ImGui::GetWindowDrawList();

                            ImGui::Image(texture, ImVec2(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE));

                            minPoint = ImGui::GetItemRectMin();
                            maxPoint = ImGui::GetItemRectMax();

                            tooltipDrawList->AddRect(
                                minPoint,
                                maxPoint,
                                IM_COL32(255, 255, 255, 255),
                                0.0f,
                                0,
                                BORDER_WIDTH
                            );
                            ImGui::SameLine();

                            savedCursor = ImGui::GetCursorPos();

                            const auto titleSize = ImGui::CalcTextSize(result->display_name().c_str());

                            ImGui::SetCursorPosY(
                                savedCursor.y + (maxPoint.y - minPoint.y - titleSize.y) / 2);

                            ImGui::TextColored(rarity_color, result->display_name().c_str());

                            ImGuiExtras::TextWrapped(result->clean_description(), 300.0f);

                            ImGui::Dummy(ImVec2(0.0f, 8.0f));

                            ImGuiExtras::Text(result->display_type());
                            ImGuiExtras::Text(result->required_level());
                            ImGuiExtras::Text(result->display_binding());

                            if (result->vendor_value > 0) {
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

                                const float textHeight = ImGui::CalcTextSize("m").y;

                                int copper = result->vendor_value * result->count;
                                int gold = copper / 10000;

                                copper %= 10000;
                                int silver = copper / 100;

                                copper %= 100;

                                savedCursor = ImGui::GetCursorPos();

                                if (gold > 0) {
                                    ImGui::TextColored(COLOR_CURRENCY_GOLD,
                                                       std::format("{}", gold).c_str());
                                    ImGui::SameLine();

                                    void *gold_texture = this->LoadResourceTexture("gold", GOLD_PNG);

                                    ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
                                    ImGui::Image(gold_texture,
                                                 ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

                                    ImGui::SameLine();
                                    ImGui::SetCursorPosY(savedCursor.y);

                                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
                                    ImGui::Dummy(ImVec2(1.0f, 0.f));
                                    ImGui::PopStyleVar();

                                    ImGui::SameLine();
                                }

                                if (silver > 0 || gold > 0) {
                                    ImGui::TextColored(COLOR_CURRENCY_SILVER,
                                                       std::format("{}", silver).c_str());
                                    ImGui::SameLine();

                                    void *silver_texture = this->LoadResourceTexture("silver", SILVER_PNG);

                                    ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
                                    ImGui::Image(silver_texture,
                                                 ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

                                    ImGui::SameLine();
                                    ImGui::SetCursorPosY(savedCursor.y);

                                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
                                    ImGui::Dummy(ImVec2(1.0f, 0.f));
                                    ImGui::PopStyleVar();

                                    ImGui::SameLine();
                                }

                                if (copper > 0 || silver > 0 || gold > 0) {
                                    ImGui::TextColored(COLOR_CURRENCY_COPPER,
                                                       std::format("{}", copper).c_str());
                                    ImGui::SameLine();

                                    void *copper_texture = this->LoadResourceTexture("copper", COPPER_PNG);

                                    ImGui::SetCursorPosY(savedCursor.y + textHeight - CURRENCY_ICON_SIZE);
                                    ImGui::Image(copper_texture,
                                                 ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

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
#pragma endregion
                    }
                }
                ImGui::PopItemWidth();
                ImGui::EndTable();

                if (!lower_dummy && hidden_bottom >= 0) {
                    // place lower dummy
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::Dummy(ImVec2(
                        window_size.x,
                        items_area_top + static_cast<float>(last_visible_row) * grid_item_size_with_spacing
                    ));
                    ImGui::PopStyleVar();

                    lower_dummy = true;
                }
            }
        }
        ImGui::EndChild();

        ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, FLT_MIN), ImVec2(400.0f, FLT_MAX));

        if (ImGui::BeginPopupModal(
            "API Key",
            &this->state->api_window,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar
        )) {
            ImGui::Text(std::format("Account ID: {0}", this->id).c_str());

            auto save = ImGui::InputText(
                "##api_key",
                this->state->key_buffer,
                IM_ARRAYSIZE(this->state->key_buffer),
                ImGuiInputTextFlags_Password | ImGuiInputTextFlags_EnterReturnsTrue
            );

            save = ImGui::Button("Save") || save;

            if (save) {
                helper::str_trim(this->state->key_buffer);

                this->config->set_api_key(this->id, this->state->key_buffer);
                this->config->save();

                this->init_or_update_client();

                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("Cancel")) {
                // restore the old api key
                strcpy_s(this->state->key_buffer, this->config->get_api_key(this->id).c_str());

                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }
    ImGui::PopStyleVar();
    ImGui::End();
}

void Finder::SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader) {
    this->load_remote_texture = texture_loader;
}

void Finder::SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader) {
    this->load_resource_texture = texture_loader;
}
