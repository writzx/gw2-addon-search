#pragma once
#include "const.h"

class Config {
    json data;
    std::filesystem::path path;

    std::vector<std::string> verified;

public:
    std::string get_api_key(const std::string &id);

    void set_api_key(const std::string &id, const std::string &api_key);

    void load();

    void save() const;

    explicit Config(const std::filesystem::path &path) {
        this->path = path;

        this->load();
    }
};
