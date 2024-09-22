#pragma once
#include <iostream>
#include <vector>

#include "ItemStore.h"
#include "Config.h"

typedef void* (*LOAD_REMOTE_TEXTURE) (const char* identifier, const char* url);
typedef void* (*LOAD_RESOURCE_TEXTURE) (const char* identifier, const int resourceId);

struct FinderState {
	char query[256];
	char key_buffer[256];

	std::map<std::string, std::vector<Item>> items;

	bool needs_refresh;
	bool can_manual_refresh;
	bool can_search;

	bool api_window;

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
private:
	Config* config;

	std::string id;
	std::string dir;

	ItemStore* store = nullptr;
	APIClient* client = nullptr;

	LOAD_REMOTE_TEXTURE load_remote_texture = nullptr;
	LOAD_RESOURCE_TEXTURE load_resource_texture = nullptr;

	std::vector<Item> items;

	std::optional<chrono::time_point> last_tick;

	void* LoadRemoteTexture(const char* identifier, const char* url) const;
	void* LoadResourceTexture(const char* identifier, int resourceId) const;

	void tick() noexcept;
	void refresh_store() noexcept;

	void init_or_update_client() {
		// todo maybe cleanup old variables
		std::string api_key = this->config->get_api_key(this->id);

		if (api_key != "") {
			if (this->client == nullptr) {
				this->client = new APIClient(this->id, api_key);
				this->store = new ItemStore(this->id, this->client, this->dir);
			} else {
				this->client->update_token(api_key);
			}

			strcpy_s(this->state->key_buffer, api_key.c_str());
		}
	}
public:
	FinderState* state;

	void InitImGui(void* ctxt, void* imgui_malloc, void* imgui_free);
	void Render();

	void SetRemoteTextureLoader(LOAD_REMOTE_TEXTURE texture_loader);
	void SetResourceTextureLoader(LOAD_RESOURCE_TEXTURE texture_loader);

	Finder(std::string id, std::string dir) {
		this->id = id;
		this->dir = dir;

		this->state = new FinderState();
		this->config = new Config(std::format("{0}\\{1}", dir, CONFIG_JSON_FILENAME));

		init_or_update_client();
	}
};
