#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>
#include <thread_pool/thread_pool.h>

#include "APIClient.h"
#include "Item.h"
#include "helper.h"

struct StoredEndpoint {
	Endpoint* ep;
	chrono::time_point last_updated;
};

class ItemStore {
private:
	chrono::duration refresh_interval;
	std::optional<chrono::time_point> last_refresh;
	std::mutex store_lock;

	dp::thread_pool<dp::details::default_function_type, std::jthread> pool;
public:
	APIClient* api_client;
	std::string dir;
	std::string status;
	bool refreshing;
	chrono::time_point last_status;

	ItemStore(APIClient* client, std::string dir):
		api_client(client),
		dir(dir),
		status("initialized."),
		refreshing(false),
		refresh_interval(MANUAL_REFRESH_INTERVAL),
		last_status(chrono::now() - 5min),
		last_refresh(std::nullopt),
		pool(dp::thread_pool(MAX_CONNECTIONS)) {}

	void refresh();
	void refresh_endpoint(Endpoint ep, SQLite::Database& stored_items);

	bool can_manual_refresh();
	bool should_auto_refresh();
	bool can_search();

	void search(std::string keyword, std::vector<Item>& results) const;

	std::vector<StoredEndpoint> endpoints();
	std::optional<chrono::time_point> last_updated();
	std::optional<chrono::time_point> last_updated(bool refresh);
};
