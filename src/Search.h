#pragma once
#include <string>
#include <variant>
#include <vector>

struct Search {
    std::string name;
    std::string thumbnail;
    std::vector<std::variant<std::string, int>> terms = {};
};
