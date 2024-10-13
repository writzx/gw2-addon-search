#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#include "const.h"

enum EndpointType {
    Default = 0,
    Character = 1,
};

struct Endpoint {
    std::string label;
    std::string path;
    EndpointType type;
};

struct EndpointComparator {
    bool operator()(const std::string &lhs, const std::string &rhs) const {
        int lhs_sort_val = INT_MAX;
        int rhs_sort_val = INT_MAX;

        if (ENDPOINT_SORT_ORDER.contains(lhs)) {
            lhs_sort_val = ENDPOINT_SORT_ORDER.at(lhs);
        } else if (lhs.starts_with("/character")) {
            lhs_sort_val = ENDPOINT_SORT_ORDER.at("/character");
        }

        if (ENDPOINT_SORT_ORDER.contains(rhs)) {
            rhs_sort_val = ENDPOINT_SORT_ORDER.at(rhs);
        } else if (rhs.starts_with("/character")) {
            rhs_sort_val = ENDPOINT_SORT_ORDER.at("/character");
        }

        return lhs_sort_val < rhs_sort_val;
    }
};

const std::map<std::string, Endpoint> ENDPOINTS = {
    {
        "/account/bank",
        {
            .label = "Bank",
            .path = "/account/bank",
            .type = Default,
        }
    },
    {
        "/account/inventory",
        {
            .label = "Shared Inventory",
            .path = "/account/inventory",
            .type = Default,
        }
    },
    {
        "/account/materials",
        {
            .label = "Materials",
            .path = "/account/materials",
            .type = Default,
        }
    }
};

class APIClient {
    httplib::Client client;

    std::map<std::string, Endpoint> _endpoints;

    std::string get_or_throw(const std::string &path);

public:
    explicit APIClient(const std::string &token): client(API_HOST_BASE) {
        this->client.set_bearer_token_auth(token);
    }

    void UpdateToken(const std::string &token);

    std::string Fetch(const std::string &endpoint_name);

    std::map<std::string, Endpoint> Endpoints(bool refresh = false);
};
