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

const std::map<std::string, Endpoint> ENDPOINTS = {
	{
		"/account/bank",
		{
			.label = "Bank",
			.path = "/account/bank",
			.type = EndpointType::Default,
		}
	},
	{
		"/account/inventory",
		{
			.label = "Shared Inventory",
			.path = "/account/inventory",
			.type = EndpointType::Default,
		}
	},
	{
		"/account/materials",
		{
			.label = "Materials",
			.path = "/account/materials",
			.type = EndpointType::Default,
		}
	}
};

class APIClient {
private:
	httplib::Client client;

	std::map<std::string, Endpoint> _endpoints;
	std::string get_or_throw(const std::string& path);
public:
	APIClient(std::string id, std::string token):
		client(API_HOST_BASE) {
		this->client.set_bearer_token_auth(token);
	}

	void update_token(std::string token);

	std::string fetch(std::string endpoint_name);

	std::map<std::string, Endpoint> endpoints(bool refresh);
	std::map<std::string, Endpoint> endpoints();
};

