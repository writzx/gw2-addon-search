#include <thread>
#include <imgui/imgui.h>

#include "Finder.h"
#include "imgui_extras.h"
#include "imgui_virtual_list.h"
#include "resource.h"

void *Finder::load_remote_texture(const char *url) const {
    auto url_str = std::string(url);
    const auto identifier = std::format("FINDER_TEX_{}", url_str.substr(url_str.find_last_of('/')));

    if (this->load_remote_tex != nullptr) {
        return this->load_remote_tex(identifier.c_str(), url);
    }

    return nullptr;
}

void *Finder::load_resource_texture(const int resourceId) const {
    const auto identifier = std::format("FINDER_TEX_{}", resourceId);

    if (this->load_resource_tex != nullptr) {
        return this->load_resource_tex(identifier.c_str(), resourceId);
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
                store->Refresh();
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

    this->state->needs_refresh = this->store->ShouldAutoRefresh();
    this->state->can_manual_refresh = this->store->CanManualRefresh();
    this->state->can_search = this->store->CanSearch();

    if (this->state->needs_refresh) {
        // refresh_store();
    }
}

void Finder::render_result_item(const Item *item) const {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();

    void *texture = this->load_remote_texture(item->icon.c_str());
    ImGui::Image(texture, ImVec2(GRID_ITEM_SIZE, GRID_ITEM_SIZE));

    const auto min_point = ImGui::GetItemRectMin();
    const auto max_point = ImGui::GetItemRectMax();

    const auto hovered = ImGui::IsItemHovered();
    auto &rarity_color = BORDER_COLORS.at(item->rarity);
    auto &rarity_color_hover = BORDER_COLORS_HOVER.at(item->rarity);

    draw_list->AddRect(
        min_point,
        max_point,
        ImGui::ColorConvertFloat4ToU32(
            hovered ? rarity_color_hover : rarity_color),
        0.0f,
        0,
        BORDER_WIDTH
    );

    const auto count = item->count_or_charges();

    if (!count.empty()) {
        ImVec2 saved_cursor = ImGui::GetCursorPos();

        const auto textSize = ImGui::CalcTextSize(count.c_str());

        ImGui::SetCursorScreenPos(ImVec2(max_point.x - textSize.x - 2.0f, min_point.y));

        ImGui::Text(count.c_str());

        ImGui::SetCursorPos(saved_cursor);
    }
    if (hovered) {
        this->render_result_item_tooltip(item);
    }
}

void Finder::render_result_item_tooltip(const Item *item) const {
    void *texture = this->load_remote_texture(item->icon.c_str());
    auto &rarity_color = BORDER_COLORS.at(item->rarity);

    ImGui::SetNextWindowSizeConstraints(
        ImVec2(FLT_MIN, FLT_MIN),
        ImVec2(300.0f, FLT_MAX)
    );

    ImGui::BeginTooltip();

    ImDrawList *tooltipDrawList = ImGui::GetWindowDrawList();

    ImGui::Image(texture, ImVec2(TOOLTIP_ICON_SIZE, TOOLTIP_ICON_SIZE));

    const auto min_point = ImGui::GetItemRectMin();
    const auto max_point = ImGui::GetItemRectMax();

    tooltipDrawList->AddRect(
        min_point,
        max_point,
        IM_COL32(255, 255, 255, 255),
        0.0f,
        0,
        BORDER_WIDTH
    );
    ImGui::SameLine();

    auto saved_cursor = ImGui::GetCursorPos();

    const auto title_size = ImGui::CalcTextSize(item->display_name().c_str());

    ImGui::SetCursorPosY(saved_cursor.y + (max_point.y - min_point.y - title_size.y) / 2);

    ImGui::TextColored(rarity_color, item->display_name().c_str());
    ImGuiExtras::TextWrapped(item->clean_description(), 300.0f);

    ImGui::Dummy(ImVec2(0.0f, 8.0f));

    ImGuiExtras::Text(item->display_type());
    ImGuiExtras::Text(item->required_level());
    ImGuiExtras::Text(item->display_binding());

    if (item->vendor_value > 0) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 4.0f));

        int copper = item->vendor_value * item->count;
        const int gold = copper / 10000;

        copper %= 10000;
        const int silver = copper / 100;

        copper %= 100;

        saved_cursor = ImGui::GetCursorPos();

        const float currency_y = ImGui::CalcTextSize("m").y + saved_cursor.y - CURRENCY_ICON_SIZE;

        if (gold > 0) {
            this->render_currency_value(
                gold,
                GOLD_PNG,
                COLOR_CURRENCY_GOLD,
                saved_cursor.y,
                currency_y
            );
        }

        if (silver > 0 || gold > 0) {
            this->render_currency_value(
                silver,
                SILVER_PNG,
                COLOR_CURRENCY_SILVER,
                saved_cursor.y,
                currency_y
            );
        }

        if (copper > 0 || silver > 0 || gold > 0) {
            this->render_currency_value(
                copper,
                COPPER_PNG,
                COLOR_CURRENCY_COPPER,
                saved_cursor.y,
                currency_y
            );
        }

        ImGui::PopStyleVar();
    }

    ImGui::EndTooltip();
}

void Finder::render_currency_value(
    int value,
    const int currency_icon_res,
    const ImVec4 color,
    float y,
    const float currency_y
) const {
    ImGui::TextColored(color, std::format("{}", value).c_str());
    ImGui::SameLine();

    void *currency_texture = this->load_resource_texture(currency_icon_res);

    ImGui::SetCursorPosY(currency_y);
    ImGui::Image(currency_texture, ImVec2(CURRENCY_ICON_SIZE, CURRENCY_ICON_SIZE));

    ImGui::SameLine();

    ImGui::SetCursorPosY(y);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::Dummy(ImVec2(1.0f, 0.f));
    ImGui::PopStyleVar();

    ImGui::SameLine();
}

void Finder::perform_search() {
    // only search if the tab changes
    if (this->last_search != this->next_search) {
        // save as last_search
        this->last_search = this->next_search;
        this->searching_or_calculating = true;

        this->state->item_sections.clear();

        this->items.clear();

        std::jthread(
            [&] {
                this->store->Search(this->next_search, this->items);

                for (auto &item: this->items) {
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

void Finder::render_saved() {
    const auto &style = ImGui::GetStyle();
    const auto window_padding = style.WindowPadding;

    void *search_texture = this->load_resource_texture(SEARCH_PNG);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(window_padding.x, 0));
    const auto saved_searches_area = ImGui::BeginChild(
        "##saved_searches",
        ImVec2(0, SAVED_SEARCH_ICON_SIZE + window_padding.y),
        false,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysUseWindowPadding
    );
    ImGui::PopStyleVar();

    if (saved_searches_area) {
        ImGui::PushID("##search_tab");
        const auto search_tab = ImGui::ImageButton(search_texture, ImVec2(SAVED_SEARCH_ICON_SIZE, SAVED_SEARCH_ICON_SIZE));
        ImGui::PopID();
        if (search_tab) {
            this->last_search = "";
            this->next_search = "";

            this->clear_search();

            this->state->item_rows.clear();
            this->state->item_sections.clear();
            this->items.clear();
        }
        ImGui::SameLine();

        for (const auto &saved_search: this->saved_searches) {
            void *s_tex = this->load_resource_texture(SEARCH_PNG);
            ImGui::PushID(std::format("##saved_search_{}", saved_search.name).c_str());
            const auto saved_search_clicked = ImGui::ImageButton(
                s_tex,
                ImVec2(SAVED_SEARCH_ICON_SIZE, SAVED_SEARCH_ICON_SIZE)
            );
            ImGui::PopID();

            ImGuiExtras::Tooltip(saved_search.name.c_str());

            if (saved_search_clicked) {
                this->next_search = saved_search.name;
                strcpy_s(this->state->query, saved_search.name.c_str());
            }

            ImGui::SameLine();
        }
    }
    ImGui::EndChild();
}

bool Finder::render_refresh() const {
    const auto &style = ImGui::GetStyle();

    const auto item_spacing = style.ItemSpacing;

    const ImVec2 saved_cursor = ImGui::GetCursorScreenPos();

    auto refresh_button = false;
    bool hovered = false;

    const bool can_refresh = this->state->can_manual_refresh && !this->store->refreshing;

    ImGuiExtras::BeginDisable(!can_refresh, true);
    if (this->store->refreshing) {
        refresh_button = ImGui::Button(
            "##refresh",
            ImVec2(ICON_BUTTON_OUTER_SIZE, ICON_BUTTON_OUTER_SIZE)
        );

        hovered = ImGui::IsItemHovered();

        ImGui::SetCursorScreenPos(ImVec2(saved_cursor.x + ICON_BUTTON_PADDING * 2,
                                         saved_cursor.y + ICON_BUTTON_PADDING * 0.5f));
        ImGuiExtras::Spinner(
            "##spinner",
            REFRESH_SPINNER_RADIUS,
            REFRESH_SPINNER_THICKNESS,
            IM_COL32(220, 220, 220, 255)
        );
    } else {
        void *refresh_texture = this->load_resource_texture(REFRESH_PNG);

        refresh_button = ImGui::ImageButton(
            refresh_texture,
            ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ICON_BUTTON_PADDING
        );

        hovered = ImGui::IsItemHovered();
    }
    ImGuiExtras::EndDisable(!can_refresh);

    ImGuiExtras::Tooltip(
        [&] {
            const auto last_updated = this->store->LastUpdated();
            ImGui::TextWrapped(
                std::format(
                    "Last Refreshed:\t{0}\nStatus:\t\t\t\t   {1}",
                    last_updated.has_value()
                        ? helper::datetime_tostring(last_updated.value())
                        : "never",
                    this->store->status
                ).c_str()
            );
        },
        hovered
    );

    ImGui::SetCursorScreenPos(ImVec2(saved_cursor.x + ICON_BUTTON_OUTER_SIZE + item_spacing.x, saved_cursor.y));

    return refresh_button && can_refresh;
}

bool Finder::render_search() const {
    const auto &style = ImGui::GetStyle();

    const auto item_spacing = style.ItemSpacing;

    void *search_texture = this->load_resource_texture(SEARCH_PNG);

    const float right_button_bar_width = ICON_BUTTON_OUTER_SIZE * 2 + item_spacing.x;

    const auto search_bar_pos = ImGui::GetCursorScreenPos();
    const auto search_bar_w =
            ImGui::GetWindowPos().x
            + ImGui::GetWindowWidth()
            - search_bar_pos.x
            - item_spacing.x
            - right_button_bar_width
            - item_spacing.x;

    ImGuiExtras::BeginDisable(!this->state->can_search);
    if (std::strlen(this->state->query) > 0) {
        const auto clear_search_pos = ImVec2(
            search_bar_pos.x + search_bar_w - ICON_BUTTON_OUTER_SIZE,
            search_bar_pos.y + 1.0f + (ImGui::GetTextLineHeightWithSpacing() - ICON_BUTTON_SIZE) *
            0.5f
        );

        ImGui::SetCursorScreenPos(clear_search_pos);

        void *backspace_texture = this->load_resource_texture(BACKSPACE_PNG);
        const auto clear_search = ImGui::InvisibleButton(
            "##clear_search",
            ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE)
        );
        ImGuiExtras::Tooltip("Clear");

        ImGui::SetCursorScreenPos(clear_search_pos);
        ImGui::Image(
            backspace_texture,
            ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE),
            ImVec2(0, 0),
            ImVec2(1, 1),
            ImGui::IsItemHovered() ? ImVec4(1.f, 1.f, 1.f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f)
        );

        if (clear_search) {
            this->clear_search();
        }
    }

    ImGui::SetCursorScreenPos(search_bar_pos);
    ImGui::SetNextItemWidth(search_bar_w);

    auto do_search = ImGui::InputText(
        "##search_input",
        this->state->query,
        IM_ARRAYSIZE(this->state->query),
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    ImGui::SameLine();

    do_search = ImGui::ImageButton(
                    search_texture,
                    ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE),
                    ImVec2(0, 0),
                    ImVec2(1, 1),
                    ICON_BUTTON_PADDING
                ) || do_search;

    ImGuiExtras::Tooltip("Search");
    ImGuiExtras::EndDisable(!this->state->can_search);

    return do_search;
}

void Finder::render_header() {
    const auto &style = ImGui::GetStyle();
    const auto window_padding = style.WindowPadding;

    const float header_height = ICON_BUTTON_OUTER_SIZE + window_padding.y * 2;

    const auto header_area = ImGui::BeginChild(
        "##header_area",
        ImVec2(0, header_height),
        false,
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_AlwaysUseWindowPadding |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse
    );
    if (header_area) {
        if (
            void *cog_texture = this->load_resource_texture(COG_PNG);
            ImGui::ImageButton(
                cog_texture,
                ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE),
                ImVec2(0, 0),
                ImVec2(1, 1),
                ICON_BUTTON_PADDING
            )
        ) {
            this->is_settings_shown = !this->is_settings_shown;
        }
        ImGuiExtras::Tooltip("Settings");

        ImGui::SameLine();

        if (this->store == nullptr || this->client == nullptr) {
            ImGui::Text("please add an api key in settings");
        } else {
            if (this->render_refresh()) {
                refresh_store();
            }

            if (this->render_search()) {
                if (
                    std::strlen(this->state->query) >= this->config->get_min_search_length()
                    && this->last_search != this->state->query
                ) {
                    helper::str_trim(this->state->query);

                    // for some reason mouse clicks on the search bar also sets do_search
                    // so we only search if the last search was different
                    this->next_search = this->state->query;
                }
            }

            ImGui::SameLine();

            void *save_texture = this->load_resource_texture(SAVE_PNG);
            const auto save_search = ImGui::ImageButton(
                save_texture,
                ImVec2(ICON_BUTTON_SIZE, ICON_BUTTON_SIZE),
                ImVec2(0, 0),
                ImVec2(1, 1),
                ICON_BUTTON_PADDING
            );
            ImGuiExtras::Tooltip("Bookmark");
            // ImGuiExtras::Tooltip("Bookmark (right-click for more options)");

            if (save_search) {
                this->saved_searches.push_back({
                    .name = this->state->query,
                    .terms = {this->state->query}
                });
            }
        }
    }
    ImGui::EndChild();
}

void Finder::recalculate_rows() {
    // account for item spacing when calculating number of columns
    const int grid_col_count = static_cast<int>(
        // exclude the cell padding from the last cell
        // half the spacing because no other cell shares the edge
        (ImGui::GetContentRegionAvail().x - GRID_ITEM_SPACING / 2)
        / (GRID_ITEM_SIZE + GRID_ITEM_SPACING + BORDER_WIDTH)
    );

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
}

void Finder::render_result_row(int i, const ItemRow &row) const {
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
        for (const auto &item: row.items) {
            ImGui::TableNextColumn();
            this->render_result_item(item);
        }
        ImGui::EndTable();
    }

    ImGui::PopStyleVar();
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

        const auto window_padding = style.WindowPadding;
        const auto item_spacing = style.ItemSpacing;

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + window_padding.y);
        this->render_saved();

        if (ImGui::BeginChild("##body")) {
            this->render_header();

            this->recalculate_rows();

            if (!this->searching_or_calculating) {
                // quick way to override item spacing at the top of the virtual list
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - item_spacing.y);

                ImGui::VirtualListV(
                    "##search_items_area",
                    // 1 extra element for the bottom padding
                    this->state->item_rows.size() + 1,
                    [&](const int i) {
                        // bottom padding
                        if (i == this->state->item_rows.size()) {
                            return SEARCH_AREA_BOTTOM_PADDING;
                        }
                        if (const auto row = state->item_rows[i]; row.label.empty()) {
                            return GRID_ITEM_SIZE;
                        }
                        return ImGui::GetTextLineHeightWithSpacing() + GRID_ITEM_SPACING;
                    },
                    [&](const int i) {
                        // bottom padding
                        if (i == this->state->item_rows.size()) {
                            return SEARCH_AREA_BOTTOM_PADDING;
                        }
                        const auto row = state->item_rows[i];
                        if (row.label.empty()) {
                            this->render_result_row(i, row);
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
        ImGui::EndChild();
    }
    ImGui::End();

    this->render_settings_window();

    this->perform_search();
}

void Finder::render_settings_window() {
    if (this->is_settings_shown) {
        ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f, FLT_MIN), ImVec2(400.0f, FLT_MAX));
        if (ImGui::Begin(
            CONFIG_POPUP_NAME.c_str(),
            &this->is_settings_shown,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
        )) {
            this->render_settings_view();
        }
        ImGui::End();
    }
}

void Finder::render_settings_view() {
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
    this->load_remote_tex = texture_loader;
}

void Finder::SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader) {
    this->load_resource_tex = texture_loader;
}

void Finder::clear_search() const {
    std::memset(&this->state->query, 0, sizeof(this->state->query));
}
