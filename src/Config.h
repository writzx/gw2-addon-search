#pragma once

#include "const.h"
#include "Search.h"

class Config {
    json data;
    std::filesystem::path path;

    std::optional<std::vector<Search>> bookmarks = std::nullopt;

public:
    std::string GetApiKey(const std::string &id);

    void SetApiKey(const std::string &id, const std::string &api_key);

    int GetMinSearchLength();

    void SetMinSearchLength(int len);

    std::vector<Search> Bookmarks(bool read = false);

    void AddBookmark(Search &bookmark);

    void RemoveBookmark(int index);

    void UpdateBookmark(int index, Search &bookmark);

    void Load();

    void Save() const;

    explicit Config(const std::filesystem::path &path) {
        this->path = path;

        this->Load();
    }
};
