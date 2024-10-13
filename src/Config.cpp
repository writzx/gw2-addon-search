#include "Config.h"
#include <fstream>

static json empty() {
    return {
        {JSON_KEY_API_KEYS, json({})},
        {JSON_KEY_MIN_SEARCH, 0},
        {JSON_KEY_BOOKMARKS, json::array()},
    };
}

std::string Config::GetApiKey(const std::string &id) {
    if (this->data[JSON_KEY_API_KEYS].contains(id)) {
        return this->data[JSON_KEY_API_KEYS][id].get<std::string>();
    }
    return "";
}

void Config::SetApiKey(const std::string &id, const std::string &api_key) {
    this->data[JSON_KEY_API_KEYS][id] = api_key;
}

std::vector<Search> Config::Bookmarks(const bool read) {
    if (!read && this->bookmarks.has_value()) {
        return this->bookmarks.value();
    }

    if (this->data[JSON_KEY_BOOKMARKS].is_array()) {
        const auto bookmarks = this->data[JSON_KEY_BOOKMARKS].get<std::vector<json>>();

        std::vector<Search> results;
        results.reserve(bookmarks.size());

        for (auto bookmark: bookmarks) {
            if (
                bookmark.is_object()
                && bookmark.contains("name")
                && bookmark.contains("thumbnail")
            ) {
                results.emplace_back(
                    bookmark["name"].get<std::string>(),
                    bookmark["thumbnail"].get<std::string>()
                );
            }
        }

        this->bookmarks = results;

        return results;
    }

    return {};
}

void Config::AddBookmark(Search &bookmark) {
    if (!this->data[JSON_KEY_BOOKMARKS].is_array()) {
        this->data[JSON_KEY_BOOKMARKS] = json::array();
    }

    this->data[JSON_KEY_BOOKMARKS].emplace_back(json({{"name", bookmark.name}, {"thumbnail", bookmark.thumbnail}}));

    if (!this->bookmarks.has_value()) {
        this->bookmarks = {bookmark};
    } else {
        this->bookmarks.value().emplace_back(bookmark);
    }
}

void Config::RemoveBookmark(const int index) {
    if (this->bookmarks.has_value() && this->bookmarks->size() > index) {
        this->bookmarks->erase(this->bookmarks->begin() + index);
    }

    if (this->data[JSON_KEY_BOOKMARKS].is_array() && this->data[JSON_KEY_BOOKMARKS].size() > index) {
        this->data[JSON_KEY_BOOKMARKS].erase(this->data[JSON_KEY_BOOKMARKS].begin() + index);
    }
}

void Config::UpdateBookmark(const int index, Search &bookmark) {
    if (this->bookmarks.has_value() && this->bookmarks->size() > index) {
        this->bookmarks.value()[index] = bookmark;
    }

    if (this->data[JSON_KEY_BOOKMARKS].is_array() && this->data[JSON_KEY_BOOKMARKS].size() > index) {
        this->data[JSON_KEY_BOOKMARKS][index] = json({{"name", bookmark.name}, {"thumbnail", bookmark.thumbnail}});
    }
}


int Config::GetMinSearchLength() {
    // default value
    int ret = 0;
    try {
        ret = this->data[JSON_KEY_MIN_SEARCH].get<int>();
    } catch (...) {
        this->data[JSON_KEY_MIN_SEARCH] = ret;
        this->Save();
    }

    return ret;
}

void Config::SetMinSearchLength(int len) {
    this->data[JSON_KEY_MIN_SEARCH] = len;
}

void Config::Save() const {
    std::ofstream config_stream(this->path);

    config_stream << std::setw(4) << this->data << std::endl;
}

void Config::Load() {
    try {
        std::ifstream config_stream(this->path);
        this->data = json::parse(config_stream);

        for (
            auto api_key_map = this->data[JSON_KEY_API_KEYS].get<std::map<std::string, std::string>>();
            auto &[id, api_key]: api_key_map
        ) {
            break;
        }
    } catch (...) {
        if (is_regular_file(this->path)) {
            std::filesystem::rename(this->path, this->path.replace_extension(".json.bak"));
        }

        this->data = empty();
        this->Save();
    }
}
