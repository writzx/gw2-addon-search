#include "Config.h"
#include <fstream>

static json empty() {
	return {
		{JSON_KEY_API_KEYS, json({})}
	};
}

std::string Config::get_api_key(std::string id) {
	if (this->data[JSON_KEY_API_KEYS].contains(id)) {
		return this->data[JSON_KEY_API_KEYS][id].get<std::string>();
	}
	return "";
}

void Config::set_api_key(std::string id, std::string api_key) {
	this->data[JSON_KEY_API_KEYS][id] = api_key;
}

void Config::save() {
	std::ofstream config_stream(this->path);

	config_stream << std::setw(4) << this->data << std::endl;
}

void Config::load() {
	try {
		std::ifstream config_stream(this->path);
		this->data = json::parse(config_stream);
		auto api_key_map = this->data[JSON_KEY_API_KEYS].get<std::map<std::string, std::string>>();

		for (auto& [id, api_key] : api_key_map) {
			break;
		}
	} catch (...) {
		if (std::filesystem::is_regular_file(this->path)) {
			std::filesystem::rename(this->path, std::format("{0}.bak", this->path));
		}

		this->data = empty();
		this->save();
	}
}
