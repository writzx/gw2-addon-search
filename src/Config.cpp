#include "Config.h"
#include <fstream>

static json empty() {
    return {
        {JSON_KEY_API_KEYS, json({})}
    };
}

std::string Config::get_api_key(const std::string &id) {
    if (this->data[JSON_KEY_API_KEYS].contains(id)) {
        return this->data[JSON_KEY_API_KEYS][id].get<std::string>();
    }
    return "";
}

void Config::set_api_key(const std::string &id, const std::string &api_key) {
    this->data[JSON_KEY_API_KEYS][id] = api_key;
}

void Config::save() const {
    std::ofstream config_stream(this->path);

    config_stream << std::setw(4) << this->data << std::endl;
}

void Config::load() {
    try {
        std::ifstream config_stream(this->path);
        this->data = json::parse(config_stream);

        for (
            auto api_key_map = this->data[JSON_KEY_API_KEYS].get<std::map<std::string, std::string> >();
            auto &[id, api_key]: api_key_map
        ) {
            break;
        }
    } catch (...) {
        if (is_regular_file(this->path)) {
            std::filesystem::rename(this->path, this->path.replace_extension(".json.bak"));
        }

        this->data = empty();
        this->save();
    }
}
