#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <thread_pool/thread_pool.h>

#include <utility>

#include "APIClient.h"
#include "Item.h"
#include "helper.h"

class ItemStore {
    chrono::duration refresh_interval;
    std::optional<chrono::time_point> last_refresh;
    std::mutex store_lock;

    dp::thread_pool<> pool;

public:
    std::string id;
    APIClient *api_client;
    std::filesystem::path dir;
    std::string status;
    bool refreshing;
    chrono::time_point last_status;

    ItemStore(
        std::string id,
        APIClient *client,
        std::filesystem::path dir
    ): refresh_interval(
           MANUAL_REFRESH_INTERVAL),
       last_refresh(std::nullopt),
       pool(dp::thread_pool(MAX_CONNECTIONS)),
       id(std::move(id)),
       api_client(client),
       dir(std::move(dir)),
       status("initialized."),
       refreshing(false),
       last_status(chrono::now() - 5min) {
    }

    void Refresh();

    void RefreshEndpoint(Endpoint ep, SQLite::Database &stored_items);

    bool CanManualRefresh();

    bool ShouldAutoRefresh() const;

    bool CanSearch() const;

    void Search(std::string keyword, std::vector<Item> &results) const;

    std::map<Endpoint *, chrono::time_point> Endpoints() const;

    std::optional<chrono::time_point> LastUpdated(bool refresh = false);
};
