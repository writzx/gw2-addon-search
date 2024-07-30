#include "APIClient.h"

string APIClient::fetch(string endpoint_name) {
	auto endpoint_element = ENDPOINTS.find(endpoint_name);

	if (endpoint_element != ENDPOINTS.end()) {
		auto& endpoint = endpoint_element->second;

		auto res = this->client.Get("/" + API_VERSION + endpoint);

		if (res && res->status == 200) {
			return res->body;
		} else {
			auto err = res.error();
			string error_string = httplib::to_string(err);

			throw error_string;
		}
	}

	throw "endpoint doesn't exist";
}
