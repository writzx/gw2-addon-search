#pragma once
#include "APIClient.h"

class ItemStore {
private:
	long refresh_interval;
	long long last_refresh;
public:
	APIClient& api_client;
	std::string path;
	ItemStore(APIClient& client, std::string path):
		api_client(client),
		path(path),
		refresh_interval(5 * 60 * 1000), // in ms
		last_refresh(-1) {
	}

	void refresh();
	std::vector<std::string> search(std::string keyword) const;
};
