#include <nlohmann/json.hpp>

#include "ItemStore.h"
#include "sql_commands.h"

using json = nlohmann::json;

static void add_to_query(json& stored_item, SQLite::Statement& insert_query, std::string ep) {
	auto item_count = stored_item["count"].get<int>();

	if (item_count <= 0) {
		// for material storage, all the items are listed with count 0
		// we don't want to store those
		return;
	}

	// despair
	insert_query.bind(1);
	insert_query.bind(2, stored_item["id"].get<int>());
	insert_query.bind(3, item_count);
	if (stored_item.contains("charges")) {
		insert_query.bind(4, stored_item["charges"].get<int>());
	} else {
		insert_query.bind(4);
	}
	if (stored_item.contains("skin")) {
		insert_query.bind(5, stored_item["skin"].get<int>());
	} else {
		insert_query.bind(5);
	}
	if (stored_item.contains("category")) {
		insert_query.bind(6, std::format("{}", stored_item["category"].get<int>()));
	} else {
		insert_query.bind(6);
	}
	if (stored_item.contains("binding")) {
		insert_query.bind(7, stored_item["binding"].get<std::string>());
	} else {
		insert_query.bind(7);
	}
	insert_query.bind(8, ep);

	insert_query.exec();
	insert_query.reset();
}

void ItemStore::refresh_endpoint(Endpoint ep, SQLite::Database& stored_items) {
	// fetch the endpoint first (before locking the store)
	json e_data = json::parse(this->api_client->fetch(ep.path));

	// lock the store to prevent other threads accessing it
	this->store_lock.lock();

	// insert endpoint if not exists
	SQLite::Statement insert_ep_query{ stored_items, INSERT_TABLE_ENDPOINTS };

	insert_ep_query.bind(1, ep.path);
	insert_ep_query.bind(2, ep.label);

	insert_ep_query.exec();

	// we reuse this query to insert all the items
	SQLite::Statement insert_query{ stored_items, INSERT_TABLE_ITEM_STORE };

	this->status = std::format("refreshing. querying endpoint: {0} ({1})...", ep.label, ep.path);
	this->last_status = chrono::now();

	// transaction to insert all items at once for the endpoint
	SQLite::Transaction transaction(stored_items);

	switch (ep.type) {
		case EndpointType::Character:
			if (e_data.contains("bags")) {
				for (json& bag : e_data["bags"]) {
					for (json& stored_item : bag["inventory"]) {
						if (!stored_item.is_null()) {
							add_to_query(stored_item, insert_query, ep.path);
						}
					}
				}
			}
			break;
		default:
			for (json& stored_item : e_data) {
				if (!stored_item.is_null()) {
					add_to_query(stored_item, insert_query, ep.path);
				}
			}
			break;
	}

	transaction.commit();

	// finally unlock the store
	this->store_lock.unlock();

	this->status = std::format("refreshing. querying endpoint: {0} ({1})...complete.", ep.label, ep.path);
	this->last_status = chrono::now();
}

bool ItemStore::can_manual_refresh() {
	auto last_update = this->last_updated(false);

	if (!last_update.has_value() ||
		chrono::now() - last_update.value() >= MANUAL_REFRESH_INTERVAL) {
		return !this->refreshing;
	}

	return false;
}

bool ItemStore::should_auto_refresh() {
	if (chrono::now() - this->last_status >= AUTO_REFRESH_INTERVAL) {
		return !this->refreshing;
	}

	return false;
}

bool ItemStore::can_search() {
	return this->last_refresh.has_value() && std::filesystem::is_regular_file(std::format("{0}\\{1}", this->dir, STORED_DB_FILENAME));
}

void ItemStore::refresh() {
	try {
		if (this->refreshing) {
			// already in progress
			return;
		}

		auto timestamp = chrono::now();

		if (this->last_refresh.has_value() && timestamp - this->last_refresh.value() < this->refresh_interval) {
			return;
		}

		this->refreshing = true;

		this->status = "refreshing...";
		this->last_status = chrono::now();

		auto stored_db_loc = std::format("{0}\\{1}", this->dir, STORED_DB_FILENAME_TEMP);
		auto stored_db_loc_final = std::format("{0}\\{1}", this->dir, STORED_DB_FILENAME);

		this->store_lock.lock();

		// create the item store if not exists
		SQLite::Database stored_items(stored_db_loc, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

		// create endpoints table if not exists
		stored_items.exec(CREATE_ENDPOINTS_TABLE);

		// create (or re-create) store items table
		stored_items.exec(CREATE_TABLE_ITEM_STORE);

		// add trigger for endpoints update
		stored_items.exec(NEW_ITEM_TRIGGER);

		this->store_lock.unlock();

		auto c_endpoints = this->api_client->endpoints(true);

		std::vector<std::future<void>> futures;

		// fetches all the endpoints from the api and then refreshes every endpoint
		for (auto& [e_name, e_endpoint] : c_endpoints) {
			futures.push_back(this->pool.enqueue([&]() { refresh_endpoint(e_endpoint, stored_items); }));
		}

		// wait for refreshes to finish
		for (auto& fut : futures) {
			fut.wait();
		}

		this->status = "refresh complete.";
		this->last_status = chrono::now();

		// copy to the final db file (and delete the temp file)
		std::filesystem::copy_file(stored_db_loc, stored_db_loc_final, std::filesystem::copy_options::overwrite_existing);
		// todo std::filesystem::remove(stored_db_loc);

		this->last_updated(true);
		this->refreshing = false;
	} catch (...) {
		this->status = "refresh failed.";
		this->last_status = chrono::now();

		this->refreshing = false;
		throw;
	}
}

void ItemStore::search(std::string keyword, std::vector<Item>& results) const {
	SQLite::Database stored_items(std::format("{0}\\{1}", this->dir, STORED_DB_FILENAME), SQLite::OPEN_READONLY);

	std::string items_db_path = this->dir + "\\items_en.db";

	std::string attach_sql = vformat(ATTACH_DB, make_format_args(items_db_path));

	stored_items.exec(attach_sql);

	std::string search_sql = vformat(QUERY_ITEM_STORE, make_format_args(keyword));

	SQLite::Statement query(stored_items, search_sql);

	while (query.executeStep()) {
		Item item = {
			.id = query.getColumn(ITEM_ID_COLUMN.c_str()),
			.storage_id = query.getColumn(ITEM_STORE_ID_COLUMN.c_str()),
			.name = query.getColumn(ITEM_NAME_COLUMN.c_str()),
			.description = query.getColumn(ITEM_DESC_COLUMN.c_str()),
			.item_level = query.getColumn(ITEM_LEVEL_COLUMN.c_str()),
			.vendor_value = query.getColumn(ITEM_VENDOR_VALUE_COLUMN.c_str()),
			.rarity = query.getColumn(ITEM_RARITY_COLUMN.c_str()),
			.type = query.getColumn(ITEM_TYPE_COLUMN.c_str()),
			.subtype = query.getColumn(ITEM_SUBTYPE_COLUMN.c_str()),
			.weight = query.getColumn(ITEM_WEIGHT_COLUMN.c_str()),
			.chat_link = query.getColumn(ITEM_CHAT_LINK_COLUMN.c_str()),
			.icon = query.getColumn(ITEM_ICON_COLUMN.c_str()),
			.count = query.getColumn(ITEM_COUNT_COLUMN.c_str()),
			.charges = query.getColumn(ITEM_CHARGES_COLUMN.c_str()),
			.skin = query.getColumn(ITEM_SKIN_COLUMN.c_str()),
			.category = query.getColumn(ITEM_CATEGORY_COLUMN.c_str()),
			.binding = query.getColumn(ITEM_BINDING_COLUMN.c_str()),
			.endpoint = query.getColumn(ENDPOINT_LABEL_COLUMN.c_str())
		};
		results.push_back(item);
	}
}

std::vector<StoredEndpoint> ItemStore::endpoints() {
	auto eps = this->api_client->endpoints();
	std::vector<StoredEndpoint> seps;

	SQLite::Database stored_items(std::format("{0}\\{1}", this->dir, STORED_DB_FILENAME), SQLite::OPEN_READONLY);

	SQLite::Statement query(stored_items, LIST_ENDPOINTS);

	while (query.executeStep()) {
		auto ep_id = query.getColumn("id");

		StoredEndpoint sep = {
			.ep = &eps[ep_id],
			.last_updated = helper::datetime_parse(query.getColumn("updated").getString())
		};

		seps.push_back(sep);
	}

	return seps;
}

std::optional<chrono::time_point> ItemStore::last_updated() {
	return this->last_updated(false);
}

std::optional<chrono::time_point> ItemStore::last_updated(bool refresh) {
	try {
		if (refresh || !this->last_refresh.has_value()) {
			for (auto const& sep : this->endpoints()) {
				if (sep.last_updated > this->last_refresh) {
					this->last_refresh = sep.last_updated;
				}
			}
		}

		if (this->last_refresh.has_value()) {
			return this->last_refresh.value();
		};
	} catch (SQLite::Exception e) {
	}
	return std::nullopt;
}
