#pragma once
#include <vector>

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
    std::string last_search = "\t";

    bool is_shown;
    bool is_settings_shown;

    bool searching_or_calculating;
    bool calculate_search_rows;

    Config *config;

    std::string id;
    std::filesystem::path dir;

    ItemStore *store = nullptr;
    APIClient *client = nullptr;

    LOAD_REMOTE_TEXTURE load_remote_texture = nullptr;
    LOAD_RESOURCE_TEXTURE load_resource_texture = nullptr;

    std::vector<Item> items;

    std::optional<chrono::time_point> last_tick;

    void *LoadRemoteTexture(const char *identifier, const char *url) const;

    void *LoadResourceTexture(const char *identifier, int resourceId) const;

    void tick() noexcept;

    void refresh_store() const noexcept;

    void draw_single_search_result(Item *item) const;

    void init_or_update_client() {
        const std::string api_key = this->config->get_api_key(this->id);

        if (!api_key.empty()) {
            if (this->client == nullptr) {
                this->client = new APIClient(api_key);
                this->store = new ItemStore(this->id, this->client, this->dir);
            } else {
                this->client->update_token(api_key);
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

    static void InitImGui(void *ctxt, void *imgui_malloc, void *imgui_free);

    void Render();

    void RenderSettingsWindow();

    void SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader);

    void SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader);

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

        init_or_update_client();
    }
};
