#pragma once
#include <map>
#include <imgui/imgui.h>
#include <regex>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using chrono = std::chrono::utc_clock;
using namespace std::chrono_literals;

constexpr int MAX_CONNECTIONS = 6;

constexpr chrono::duration UI_TICK_INTERVAL = 5s;
constexpr chrono::duration AUTO_REFRESH_INTERVAL = 2min;
constexpr chrono::duration MANUAL_REFRESH_INTERVAL = 1min;

constexpr float BORDER_WIDTH = 1.0f;
constexpr float GRID_ITEM_SIZE = 48.0f;
constexpr float TOOLTIP_ICON_SIZE = 32.0f;
constexpr float CURRENCY_ICON_SIZE = 15.0f;
constexpr float REFRESH_BUTTON_SIZE = 24.0f;
constexpr float REFRESH_SPINNER_RADIUS = 8.0f;
constexpr float REFRESH_SPINNER_THICKNESS = 2.0f;

const std::regex HTML_TAGS_PATTERN("<[^>]*>");

const std::string DATETIME_FORMAT = "%Y-%m-%d %H:%M:%S";

const auto COLOR_CURRENCY_GOLD = ImVec4(246 / 255.f, 206 / 255.f, 91 / 255.f, 255 / 255.f);
const auto COLOR_CURRENCY_SILVER = ImVec4(199 / 255.f, 197 / 255.f, 199 / 255.f, 255 / 255.f);
const auto COLOR_CURRENCY_COPPER = ImVec4(170 / 255.f, 90 / 255.f, 23 / 255.f, 255 / 255.f);

const std::map<std::string, ImVec4> BORDER_COLORS = {
    {"Junk", ImVec4(170 / 255.0f, 170 / 255.0f, 170 / 255.0f, 255 / 255.0f)},
    {"Basic", ImVec4(255, 255, 255, 255 / 255.0f)},
    {"Fine", ImVec4(98 / 255.0f, 164 / 255.0f, 218 / 255.0f, 255 / 255.0f)},
    {"Masterwork", ImVec4(26 / 255.0f, 147 / 255.0f, 6 / 255.0f, 255 / 255.0f)},
    {"Rare", ImVec4(252 / 255.0f, 208 / 255.0f, 11 / 255.0f, 255 / 255.0f)},
    {"Exotic", ImVec4(255 / 255.0f, 164 / 255.0f, 5 / 255.0f, 255 / 255.0f)},
    {"Ascended", ImVec4(251 / 255.0f, 62 / 255.0f, 141 / 255.0f, 255 / 255.0f)},
    {"Legendary", ImVec4(76 / 255.0f, 19 / 255.0f, 157 / 255.0f, 255 / 255.0f)},
};

const std::map<std::string, ImVec4> BORDER_COLORS_HOVER = {
    {"Junk", ImVec4(170 / 255.0f, 170 / 255.0f, 170 / 255.0f, 172 / 255.0f)},
    {"Basic", ImVec4(255, 255, 255, 172 / 255.0f)},
    {"Fine", ImVec4(98 / 255.0f, 164 / 255.0f, 218 / 255.0f, 172 / 255.0f)},
    {"Masterwork", ImVec4(26 / 255.0f, 147 / 255.0f, 6 / 255.0f, 172 / 255.0f)},
    {"Rare", ImVec4(252 / 255.0f, 208 / 255.0f, 11 / 255.0f, 172 / 255.0f)},
    {"Exotic", ImVec4(255 / 255.0f, 164 / 255.0f, 5 / 255.0f, 172 / 255.0f)},
    {"Ascended", ImVec4(251 / 255.0f, 62 / 255.0f, 141 / 255.0f, 172 / 255.0f)},
    {"Legendary", ImVec4(76 / 255.0f, 19 / 255.0f, 157 / 255.0f, 172 / 255.0f)},
};

const std::string ADDON_NAME = "Finder";
const std::string ITEMS_DB_FILENAME = "items_en.db";
const std::string STORED_DB_FILENAME = "stored_items.db";
const std::string STORED_DB_FILENAME_TEMP = "stored_items_temp.db";
const std::string CONFIG_JSON_FILENAME = "config.json";

const std::string JSON_KEY_API_KEYS = "api_keys";

const std::string API_HOST_BASE = "https://api.guildwars2.com";
const std::string API_VERSION = "v2";

const std::string RENDER_HOST_BASE = "https://render.guildwars2.com";

const std::map<std::string, int> ENDPOINT_SORT_ORDER = {
    {"/account/bank", 1}, {"/account/inventory", 2}, {"/character", 3}, {"/account/materials", 4}
};
