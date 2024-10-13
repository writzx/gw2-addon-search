#pragma once
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct Search {
    std::string name;
    std::string thumbnail;

    Search(
        std::string name,
        std::string thumbnail
    ): name(std::move(name)),
       thumbnail(std::move(thumbnail)) {
    }
};
