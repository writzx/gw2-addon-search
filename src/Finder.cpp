#include "Finder.h"
#include <imgui/imgui.h>
#include <thread>

#include "imgui_extras.h"
#include "imgui_virtual_list.h"
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

void Finder::draw_single_search_result(Item *item) const {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 savedCursor;

    void *texture = this->LoadRemoteTexture(
        std::format("ITEM_TEX_{}", item->id).c_str(),
        item->icon.c_str()
    );
    ImGui::Image(texture, ImVec2(GRID_ITEM_SIZE, GRID_ITEM_SIZE));

    auto minPoint = ImGui::GetItemRectMin();
    auto maxPoint = ImGui::GetItemRectMax();

    std::string rarity = item->rarity;

    const auto hovered = ImGui::IsItemHovered();
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

    const auto count = item->count_or_charges();

    if (!count.empty()) {
        savedCursor = ImGui::GetCursorPos();

        const auto textSize = ImGui::CalcTextSize(count.c_str());

        ImGui::SetCursorScreenPos(ImVec2(maxPoint.x - textSize.x - 2.0f, minPoint.y));

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

        const auto titleSize = ImGui::CalcTextSize(item->display_name().c_str());

        ImGui::SetCursorPosY(
            savedCursor.y + (maxPoint.y - minPoint.y - titleSize.y) / 2);

        ImGui::TextColored(rarity_color, item->display_name().c_str());

        ImGuiExtras::TextWrapped(item->clean_description(), 300.0f);

        ImGui::Dummy(ImVec2(0.0f, 8.0f));

        ImGuiExtras::Text(item->display_type());
        ImGuiExtras::Text(item->required_level());
        ImGuiExtras::Text(item->display_binding());

        if (item->vendor_value > 0) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

            const float textHeight = ImGui::CalcTextSize("m").y;

            int copper = item->vendor_value * item->count;
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

void Finder::Render() {
    if (this->store != nullptr) {
        this->tick();
    }

    if (!this->is_shown) {
        return;
    }

    ImGui::SetNextWindowSizeConstraints(ImVec2(420.0f, 300.0f), ImVec2(FLT_MAX, FLT_MAX));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    const auto finder = ImGui::Begin(ADDON_NAME.c_str(), &this->is_shown, ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    if (finder) {
        const auto &style = ImGui::GetStyle();

        const auto window_size = ImGui::GetContentRegionAvail();

        const auto frame_padding = style.FramePadding;
        const auto window_padding = style.WindowPadding;
        const auto item_spacing = style.ItemSpacing;

        // account for item spacing when calculating number of columns
        const int grid_col_count = static_cast<int>(
            // exclude the cell padding from the last cell
            // half the spacing because no other cell shares the edge
            (window_size.x - GRID_ITEM_SPACING / 2)
            / (GRID_ITEM_SIZE + GRID_ITEM_SPACING + BORDER_WIDTH)
        );
        const float search_bar_height = REFRESH_BUTTON_SIZE + window_padding.y * 2;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, window_padding);

        const auto header_area = ImGui::BeginChild(
            "##header_area",
            ImVec2(0, search_bar_height),
            false,
            ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysUseWindowPadding |
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
        );

        ImGui::PopStyleVar();

        if (header_area) {
            ImGuiExtras::BeginDisable(this->store != nullptr && this->client != nullptr && this->store->refreshing);

            if (ImGui::Button("API", ImVec2(0, REFRESH_BUTTON_SIZE))) {
                this->is_settings_shown = true;
            }

            ImGuiExtras::EndDisable(this->store != nullptr && this->client != nullptr && this->store->refreshing);

            ImGui::SameLine();

            if (this->store == nullptr || this->client == nullptr) {
                ImGui::Text("please add an api key");
            } else {
#pragma region refresh button
                auto refreshBtn = false;

                const bool can_refresh = this->state->can_manual_refresh && !this->store->refreshing;

                ImGuiExtras::BeginDisable(!can_refresh, true);

                const ImVec2 savedCursor = ImGui::GetCursorScreenPos();
                refreshBtn = ImGui::Button("##refresh", ImVec2(REFRESH_BUTTON_SIZE, REFRESH_BUTTON_SIZE));

                const bool hovered = ImGui::IsItemHovered();

                const auto spinnerLocation = savedCursor - frame_padding
                                             - REFRESH_SPINNER_RADIUS + REFRESH_BUTTON_SIZE * 0.5;

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

                ImGuiExtras::EndDisable(!can_refresh);

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

                const float search_button_width = ImGui::CalcTextSize("Search").x + style.FramePadding.x * 2.f;

                ImGuiExtras::BeginDisable(!this->state->can_search);

                const auto search_bar_x = spinnerLocation.x + REFRESH_BUTTON_SIZE + item_spacing.x;
                ImGui::SetCursorScreenPos(ImVec2(search_bar_x, spinnerLocation.y));

                ImGui::SetNextItemWidth(
                    ImGui::GetWindowPos().x
                    + window_size.x
                    - search_bar_x
                    - item_spacing.x
                    - search_button_width
                    - item_spacing.x
                );

                auto do_search = ImGui::InputText(
                    "##search_input",
                    this->state->query,
                    IM_ARRAYSIZE(this->state->query),
                    ImGuiInputTextFlags_EnterReturnsTrue
                );

                ImGui::SameLine();

                do_search = ImGui::Button("Search", ImVec2(search_button_width, REFRESH_BUTTON_SIZE)) || do_search;

                ImGuiExtras::EndDisable(!this->state->can_search);

                if (do_search) {
                    helper::str_trim(this->state->query);

                    if (
                        std::strlen(this->state->query) >= this->config->get_min_search_length()
                        && this->last_search != this->state->query
                    ) {
                        // for some reason mouse clicks on the search bar also sets do_search
                        // so we only search if the last search was different
                        this->last_search = this->state->query;
                        this->searching_or_calculating = true;

                        this->state->item_sections.clear();

                        items.clear();

                        std::jthread(
                            [&] {
                                this->store->search(this->state->query, items);

                                for (auto &item: items) {
                                    if (!this->state->item_sections.contains(item.endpoint_path)) {
                                        this->state->item_sections[item.endpoint_path] = {};
                                    }
                                    this->state->item_sections[item.endpoint_path].push_back(&item);
                                }
                                this->searching_or_calculating = false;
                                this->calculate_search_rows = true;
                            }
                        ).detach();
                    }
                }
            }
        }
        ImGui::EndChild();

        if (!this->searching_or_calculating && this->state->col_count != grid_col_count) {
            this->state->col_count = grid_col_count;
            this->calculate_search_rows = true;
        }

        if (this->calculate_search_rows && !this->searching_or_calculating) {
            this->state->item_rows.clear();
            this->searching_or_calculating = true;

            std::jthread(
                [&] {
                    for (auto &sec_items: this->state->item_sections | std::views::values) {
                        if (sec_items.empty()) { continue; }
                        this->state->item_rows.push_back({
                            .label = sec_items[0]->endpoint,
                            .items = {}
                        });

                        for (int i = 0; i < sec_items.size(); i += this->state->col_count) {
                            auto start = sec_items.begin() + i;
                            const auto end = i + this->state->col_count < sec_items.size()
                                                 ? start + this->state->col_count
                                                 : sec_items.end();

                            if (const std::vector<Item *> sub_items = {start, end}; !sub_items.empty()) {
                                this->state->item_rows.push_back({
                                    .label = "",
                                    .items = sub_items
                                });
                            }
                        }
                    }

                    this->calculate_search_rows = false;
                    this->searching_or_calculating = false;
                }
            ).detach();
        }

        if (!this->searching_or_calculating) {
            // quick way to override item spacing at the top of the virtual list
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing.y);

            ImGui::VirtualListV(
                "##search_items_area",
                // 1 extra element for the bottom padding
                this->state->item_rows.size() + 1,
                [&](const int i) {
                    if (i == this->state->item_rows.size()) {
                        return SEARCH_AREA_BOTTOM_PADDING;
                    }
                    if (const auto row = state->item_rows[i]; row.label.empty()) {
                        return GRID_ITEM_SIZE;
                    }
                    return ImGui::GetTextLineHeightWithSpacing() + GRID_ITEM_SPACING;
                },
                [&](const int i) {
                    if (i == this->state->item_rows.size()) {
                        return SEARCH_AREA_BOTTOM_PADDING;
                    }
                    const auto row = state->item_rows[i];
                    if (row.label.empty()) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + GRID_ITEM_SPACING);
                        // half the spacing to account for both cells sharing an edge!
                        ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(GRID_ITEM_SPACING / 2, 0));

                        const auto table = ImGui::BeginTable(
                            std::format("##table_{}", i).c_str(),
                            this->state->col_count
                        );

                        if (table) {
                            for (int col = 0; col < this->state->col_count; col++) {
                                ImGui::TableSetupColumn(
                                    std::format("##col_{}", col).c_str(),
                                    ImGuiTableColumnFlags_WidthFixed,
                                    GRID_ITEM_SIZE
                                );
                            }
                            for (auto &item: row.items) {
                                ImGui::TableNextColumn();
                                draw_single_search_result(item);
                            }
                            ImGui::EndTable();
                        }

                        ImGui::PopStyleVar();

                        return GRID_ITEM_SIZE;
                    }
                    // ImGui::SetCursorPosX(ImGui::GetCursorPosX() + window_padding.x);
                    ImGui::CollapsingHeader(
                        row.label.c_str(),
                        ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Leaf
                    );
                    return ImGui::GetTextLineHeightWithSpacing() + GRID_ITEM_SPACING;
                },
                GRID_ITEM_SPACING
            );
        }
    }
    ImGui::End();

    if (this->is_settings_shown) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, FLT_MIN), ImVec2(400.0f, FLT_MAX));
        if (ImGui::Begin(
            CONFIG_POPUP_NAME.c_str(),
            &this->is_settings_shown,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
        )) {
            this->RenderSettingsWindow();
        }
        ImGui::End();
    }
}

void Finder::RenderSettingsWindow() {
    ImGui::LabelText(ImGuiExtras::LeftAlignedLabel("Account ID:").c_str(), std::format("{}", this->id).c_str());

    if (ImGui::InputText(
        ImGuiExtras::LeftAlignedLabel("API Key: ").c_str(),
        this->settings_state->api_key_buffer,
        IM_ARRAYSIZE(this->settings_state->api_key_buffer),
        ImGuiInputTextFlags_Password
    )) {
        this->settings_state->pending_save = true;
    }

    if (ImGui::SliderInt(
        ImGuiExtras::LeftAlignedLabel("Min. search length: ").c_str(),
        &this->settings_state->min_search_length,
        0,
        10
    )) {
        this->settings_state->pending_save = true;
    }

    ImGuiExtras::BeginDisable(!this->settings_state->pending_save);
    if (ImGui::Button("Save")) {
        helper::str_trim(this->settings_state->api_key_buffer);

        this->config->set_api_key(this->id, this->settings_state->api_key_buffer);
        this->config->set_min_search_length(this->settings_state->min_search_length);

        this->config->save();

        this->settings_state->pending_save = false;

        this->init_or_update_client();
    }
    ImGuiExtras::EndDisable(!this->settings_state->pending_save);

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        // restore from the saved config
        strcpy_s(this->settings_state->api_key_buffer, this->config->get_api_key(this->id).c_str());
        this->settings_state->min_search_length = this->config->get_min_search_length();

        this->settings_state->pending_save = false;
        this->is_settings_shown = false;
    }
}

void Finder::SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader) {
    this->load_remote_texture = texture_loader;
}

void Finder::SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader) {
    this->load_resource_texture = texture_loader;
}
