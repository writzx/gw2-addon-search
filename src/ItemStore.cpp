#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

#include "ItemStore.h"
#include "sql_commands.h"

using json = nlohmann::json;

static long long current_millis() {
	auto now = chrono::system_clock::now().time_since_epoch();

	// Convert duration to milliseconds
	return chrono::duration_cast<chrono::milliseconds>(now).count();
}

void ItemStore::refresh() {
	auto timestamp = current_millis();

	if (this->last_refresh >= 0 && timestamp - this->last_refresh < this->refresh_interval) {
		return;
	}

	SQLite::Database stored_items("stored_items.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

	stored_items.exec(CREATE_TABLE_ITEM_STORE);

	SQLite::Statement insert_query{ stored_items, INSERT_TABLE_ITEM_STORE };

	for (auto const& [e_name, e_url] : ENDPOINTS) {
		json e_data = json::parse(this->api_client.fetch(e_name));

		SQLite::Transaction transaction(stored_items);

		for (json& stored_item : e_data) {
			if (!stored_item.is_null()) {
				// despair
				insert_query.bind(1);
				insert_query.bind(2, stored_item["id"].template get<int>());
				insert_query.bind(3, stored_item["count"].template get<int>());
				if (stored_item.contains("charges")) {
					insert_query.bind(4, stored_item["charges"].template get<int>());
				} else {
					insert_query.bind(4);
				}
				if (stored_item.contains("skin")) {
					insert_query.bind(5, stored_item["skin"].template get<int>());
				} else {
					insert_query.bind(5);
				}
				if (stored_item.contains("category")) {
					insert_query.bind(6, format("{}", stored_item["category"].template get<int>()));
				} else {
					insert_query.bind(6);
				}
				if (stored_item.contains("binding")) {
					insert_query.bind(7, stored_item["binding"].template get<string>());
				} else {
					insert_query.bind(7);
				}
				insert_query.bind(8, e_url);

				insert_query.exec();
				insert_query.reset();
			}
		}

		transaction.commit();
	}

	stored_items.~Database();
}
