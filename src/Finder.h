#pragma once
#include <vector>

#include "Search.h"
#include "ItemStore.h"
#include "Config.h"

typedef void * (*LOAD_REMOTE_TEXTURE)(const char *identifier, const char *url);

typedef void * (*LOAD_RESOURCE_TEXTURE)(const char *identifier, int resourceId);

struct ItemRow {
    std::string label;
    std::vector<Item *> items;
};

struct SettingsState {
    char api_key_buffer[256];
    int min_search_length;

    bool pending_save;

    // NOLINTNEXTLINE(*-pro-type-member-init)
    SettingsState() {
        std::memset(&this->api_key_buffer, 0, sizeof(this->api_key_buffer));
        this->min_search_length = 0;

        this->pending_save = false;
    }
};

struct FinderState {
    char query[256];

    std::map<std::string, std::vector<Item *>> item_sections = {};
    std::vector<ItemRow> item_rows = {};

    bool needs_refresh;
    bool can_manual_refresh;
    bool can_search;

    int col_count;

    // NOLINTNEXTLINE(*-pro-type-member-init)
    FinderState() {
        std::memset(&this->query, 0, sizeof(this->query));

        this->needs_refresh = true;
        this->can_manual_refresh = true;
        this->can_search = true;

        this->col_count = 0;
    }
};


class Finder {
    // todo std::vector<std::variant<std::string, int>>
    std::string last_search = "\t";
    std::string next_search = "\t";

    bool is_shown;
    bool is_settings_shown;

    bool is_showing_bookmark = false;

    bool searching_or_calculating;
    bool calculate_search_rows;

    Config *config;

    std::string id;
    std::filesystem::path dir;

    ItemStore *store = nullptr;
    APIClient *client = nullptr;

    LOAD_REMOTE_TEXTURE load_remote_tex = nullptr;
    LOAD_RESOURCE_TEXTURE load_resource_tex = nullptr;

    std::vector<Item> items = {};

    float search_results_total_height = -1;

    std::optional<chrono::time_point> last_tick;

    void *load_remote_texture(const char *) const;

    void *load_resource_texture(int) const;

    bool render_search() const;

    bool render_refresh() const;

    void render_settings_window();

    void recalculate_rows();

    void render_result_row(int, const ItemRow &) const;

    void render_result_item_menu(const Item *item) const;

    void tick() noexcept;

    void refresh_store() const noexcept;

    void render_result_item(const Item *) const;

    void render_result_item_tooltip(const Item *) const;

    void render_currency_value(int, int, ImVec4, float, float) const;

    void render_bookmarks();

    void clear_search() const;

    void perform_search();

    void render_header();

    void init_or_update_client() {
        const std::string api_key = this->config->GetApiKey(this->id);

        if (!api_key.empty()) {
            if (this->client == nullptr) {
                this->client = new APIClient(api_key);
                this->store = new ItemStore(this->id, this->client, this->dir);
            } else {
                this->client->UpdateToken(api_key);
            }

            strcpy_s(this->settings_state->api_key_buffer, api_key.c_str());
        }
    }

public:
    FinderState *state;
    SettingsState *settings_state;

    void Show();

    void Hide();

    void Toggle();

    static void InitImGui(void *, void *, void *);

    void Render();

    void RenderSettingsView();

    void SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE);

    void SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE);

    Finder(const std::string &id, const std::filesystem::path &dir) {
        this->id = id;
        this->dir = dir;

        this->state = new FinderState();
        this->settings_state = new SettingsState();

        this->is_shown = false;
        this->is_settings_shown = false;

        this->searching_or_calculating = false;
        this->calculate_search_rows = false;

        this->config = new Config(dir / CONFIG_JSON_FILENAME);

        this->settings_state->min_search_length = this->config->GetMinSearchLength();

        this->init_or_update_client();
    }
};
