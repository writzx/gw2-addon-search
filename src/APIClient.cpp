#include "APIClient.h"

std::string APIClient::get_or_throw(const std::string& path) {
	auto res = this->client.Get(path);

	if (res && res->status == 200) {
		return res->body;
	} else {
		auto err = res.error();
		std::string error_string = httplib::to_string(err);

		throw error_string;
	}
}

void APIClient::update_token(std::string token) {
	this->client.set_bearer_token_auth(token);
}

std::string APIClient::fetch(std::string endpoint_name) {
	auto endpoints = this->endpoints();
	auto endpoint = endpoints.find(endpoint_name);

	if (endpoint != endpoints.end()) {
		return this->get_or_throw("/" + API_VERSION + endpoint->second.path);
	}

	throw "endpoint doesn't exist";
}

std::map<std::string, Endpoint> APIClient::endpoints(bool refresh) {
	if (refresh) {
		this->_endpoints.clear();

		for (auto const& [id, ep] : ENDPOINTS) {
			this->_endpoints[id] = ep;
		}

		json characters = json::parse(this->get_or_throw("/" + API_VERSION + "/characters"));

		for (json& character : characters) {
			std::string charName = character.get<std::string>();

			Endpoint ep = {
				.label = charName,
				.path = "/characters/" + charName + "/inventory",
				.type = EndpointType::Character,
			};

			this->_endpoints[ep.path] = ep;
		}
	}

	return this->_endpoints;
}

std::map<std::string, Endpoint> APIClient::endpoints() {
	return this->endpoints(false);
}
