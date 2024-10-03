#pragma once
#include <vector>

#include "ItemStore.h"
#include "Config.h"

typedef void * (*LOAD_REMOTE_TEXTURE)(const char *identifier, const char *url);

typedef void * (*LOAD_RESOURCE_TEXTURE)(const char *identifier, int resourceId);

struct ItemSection {
    std::string label;
    std::vector<Item *> items;

    // top + header_height + section_height (header_height is same for all sections)
    float top = 0;
    float items_area_height = 0;
};

struct FinderState {
    char query[256];
    char key_buffer[256];

    std::map<std::string, ItemSection> item_sections = {};

    bool needs_refresh;
    bool can_manual_refresh;
    bool can_search;

    bool api_window;

    // NOLINTNEXTLINE(*-pro-type-member-init)
    FinderState() {
        std::memset(&this->query, 0, sizeof(this->query));
        std::memset(&this->key_buffer, 0, sizeof(this->key_buffer));

        this->needs_refresh = true;
        this->can_manual_refresh = true;
        this->can_search = true;

        this->api_window = false;
    }
};


class Finder {
    bool is_shown;

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

    void init_or_update_client() {
        const std::string api_key = this->config->get_api_key(this->id);

        if (!api_key.empty()) {
            if (this->client == nullptr) {
                this->client = new APIClient(api_key);
                this->store = new ItemStore(this->id, this->client, this->dir);
            } else {
                this->client->update_token(api_key);
            }

            strcpy_s(this->state->key_buffer, api_key.c_str());
        }
    }

public:
    FinderState *state;

    void Show();

    void Hide();

    void Toggle();

    static void InitImGui(void *ctxt, void *imgui_malloc, void *imgui_free);

    void Render();

    void SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader);

    void SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader);

    Finder(const std::string &id, const std::filesystem::path &dir) {
        this->id = id;
        this->dir = dir;

        this->is_shown = false;

        this->state = new FinderState();
        this->config = new Config(dir / CONFIG_JSON_FILENAME);

        init_or_update_client();
    }
};
