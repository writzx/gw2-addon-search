import sqlite3
import json
import requests

API_BASE_URL = "https://api.guildwars2.com/v2"
BATCH_SIZE = 100


def batch_item_ids(all_ids, n=BATCH_SIZE):
    for i in range(0, len(all_ids), n):
        yield all_ids[i:i + n]


def main():
    items_res = requests.get(f"{API_BASE_URL}/items")

    if not items_res.ok:
        print("failed to fetch item ids")
        return

    item_ids = items_res.json()

    for batch_ids in batch_item_ids(item_ids):
        items = requests.get(f"{API_BASE_URL}/items?ids={",".join([f'{i}' for i in batch_ids])}")
        return


if __name__ == '__main__':
    main()
