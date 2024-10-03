#pragma once
#include "const.h"

const std::map<std::string, std::map<std::string, std::string>> DISPLAY_TYPE_MAP = {
    {
        "Armor",
        {
            {"Helm", "Head Armor"},
            {"Shoulders", "Shoulder Armor"},
            {"Coat", "Chest Armor"},
            {"Gloves", "Hand Armor"},
            {"Leggings", "Leg Armor"},
            {"Boots", "Foot Armor"}
        }
    },
    {
        "CraftingMaterial",
        {{"", "Crafting Material"}}
    }
};

struct Item {
    int id;
    int storage_id;

    std::string name;
    std::string description;

    int item_level;
    int vendor_value;

    std::string rarity;
    std::string type;
    std::string subtype;

    std::string weight;
    std::string chat_link;

    std::string icon;

    int count;

    int charges;
    int skin;
    int category;

    std::string binding;

    std::string endpoint_path;
    std::string endpoint;

    std::string display_name() const {
        if (count > 1) {
            return std::format("{1} {0}", name, count);
        }

        return name;
    }

    std::string clean_description() const {
        return std::regex_replace(description, HTML_TAGS_PATTERN, "");
    }

    std::string display_binding() const {
        if (binding == "Account") {
            return "Account Bound";
        }

        if (binding == "Character") {
            return "Soulbound";
        }

        return binding;
    }

    std::string display_type() const {
        if (DISPLAY_TYPE_MAP.contains(type)) {
            auto &subtype_map = DISPLAY_TYPE_MAP.at(type);

            // if subtype map contains empty key,
            // that is the only value for the type
            if (subtype_map.contains("")) {
                return subtype_map.at("");
            }

            if (subtype_map.contains(subtype)) {
                return subtype_map.at(subtype);
            }

            return subtype;
        }

        return type;
    }

    std::string required_level() const {
        if (item_level > 0) {
            return std::format("Required Level: {0}", item_level);
        }
        return "";
    }

    std::string count_or_charges() const {
        auto c = count > 1 ? count : charges > 1 ? charges : 0;

        if (c == 0) {
            return "";
        }

        return std::format("{}", c);
    }
};
