#include "APIClient.h"

std::string APIClient::get_or_throw(const std::string &path) {
    if (auto res = this->client.Get(path); res && res->status == 200) {
        return res->body;
    } else {
        const auto err = res.error();
        std::string error_string = to_string(err);

        throw error_string;
    }
}

void APIClient::update_token(const std::string &token) {
    this->client.set_bearer_token_auth(token);
}

std::string APIClient::fetch(const std::string &endpoint_name) {
    auto endpoints = this->endpoints();

    if (const auto endpoint = endpoints.find(endpoint_name); endpoint != endpoints.end()) {
        return this->get_or_throw("/" + API_VERSION + endpoint->second.path);
    }

    throw "endpoint doesn't exist";
}

std::map<std::string, Endpoint> APIClient::endpoints(const bool refresh) {
    if (refresh) {
        this->_endpoints.clear();

        for (auto const &[id, ep]: ENDPOINTS) {
            this->_endpoints[id] = ep;
        }

        for (
            json characters = json::parse(this->get_or_throw("/" + API_VERSION + "/characters"));
            json &character: characters
        ) {
            auto charName = character.get<std::string>();

            Endpoint ep = {
                .label = charName,
                .path = "/characters/" + charName + "/inventory",
                .type = Character,
            };

            this->_endpoints[ep.path] = ep;
        }
    }

    return this->_endpoints;
}

std::map<std::string, Endpoint> APIClient::endpoints() {
    return this->endpoints(false);
}
