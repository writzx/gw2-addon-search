#pragma once
#include <iostream>
#include "const.h"

class Config {
private:
	json data;
	std::filesystem::path path;

	std::vector<std::string> verified;
public:
	std::string get_api_key(std::string id);
	void set_api_key(std::string id, std::string api_key);

	void load();
	void save();

	Config(std::filesystem::path path) {
		this->path = path;

		this->load();
	}
};
