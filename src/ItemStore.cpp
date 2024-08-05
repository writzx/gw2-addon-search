#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

#include "ItemStore.h"
#include "sql_commands.h"

#include "helper.h"

using json = nlohmann::json;

void ItemStore::refresh() {
	auto timestamp = helper::current_millis();

	if (this->last_refresh >= 0 && timestamp - this->last_refresh < this->refresh_interval) {
		return;
	}

	auto stored_db_loc = std::format("{0}\\{1}", this->path, "stored_items.db");

	SQLite::Database stored_items(stored_db_loc, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

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
					insert_query.bind(6, std::format("{}", stored_item["category"].template get<int>()));
				} else {
					insert_query.bind(6);
				}
				if (stored_item.contains("binding")) {
					insert_query.bind(7, stored_item["binding"].template get<std::string>());
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
}

std::vector<std::string> ItemStore::search(std::string keyword) const {
	SQLite::Database stored_items(this->path + "\\stored_items.db", SQLite::OPEN_READONLY);

	std::string items_db_path = this->path + "\\items_en.db";

	std::string attach_sql = vformat(ATTACH_DB, make_format_args(items_db_path));

	stored_items.exec(attach_sql);

	std::string search_sql = vformat(QUERY_ITEM_STORE, make_format_args(keyword));

	SQLite::Statement query(stored_items, search_sql);

	std::vector<std::string> results;

	while (query.executeStep()) {
		results.push_back(query.getColumn(ITEM_NAME_COLUMN.c_str()));
	}

	return results;
}
