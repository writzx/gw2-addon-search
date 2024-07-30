import sys
import sqlite3
import traceback
import argparse

from requests import Session
from requests.adapters import HTTPAdapter, Retry

API_BASE_URL = "https://api.guildwars2.com/v2"
BATCH_SIZE = 100

HEADERS = {
    "Authorization": "Bearer 84D75703-75B3-1A49-AB5A-D48B60AADBD36C8F4DF7-7858-4CFF-9258-5D985F191728"
}

session = Session()

retries = Retry(
    total=5,
    backoff_factor=0.1,
    status_forcelist=[500, 502, 503, 504]
)

session.mount(API_BASE_URL, HTTPAdapter(max_retries=retries))

session.headers = HEADERS

ENDPOINTS = [
    {
        "name": "Bank",
        "url": "/account/bank"
    },
    {
        "name": "Shared Inventory",
        "url": "/account/inventory"
    },
    {
        "name": "Materials",
        "url": "/account/materials"
    }
]

STORED_ITEMS_TABLE_NAME = "stored_items"

CREATE_TABLE_STORED_ITEM = f"""
drop table if exists {STORED_ITEMS_TABLE_NAME};
create table {STORED_ITEMS_TABLE_NAME} (
    id              integer     primary key,
    item_id         integer     not null,
    count           int         not null,
    
    charges         int,
    
    skin            integer,
    category        integer,
    binding         text,
    
    endpoint        text
);
"""

INSERT_TABLE_STORED_ITEMS = f"""
insert into {STORED_ITEMS_TABLE_NAME}
(id, item_id, count, charges, skin, category, binding, endpoint)
values (?,?,?,?,?,?,?,?)
"""

ATTACH_DB = f"""
attach 'items_en.db' as items_db;
"""
# attach 'stored_items.db' as stored_items_db;

QUERY_STORED_ITEMS = f"""
select * from 
    {STORED_ITEMS_TABLE_NAME} sdb
        inner join
    items_db.items_en idb
        on idb.id = sdb.item_id
    where idb.name like '%{{query}}%';
"""


# QUERY_STORED_ITEMS = f"""
# select * from
#     stored_items_db.{STORED_ITEMS_TABLE_NAME} sdb
#         inner join
#     items_db.items_en idb
#         on idb.id = sdb.item_id
#     where idb.name like '%{{query}}%';
# """


def fetch_all_items():
    try:
        print(f"generating stored items db.")
        with sqlite3.connect(database=f"stored_items.db") as conn:
            cursor = conn.cursor()

            cursor.executescript(CREATE_TABLE_STORED_ITEM)

            conn.commit()
            cursor.close()

            print("fetching stored items … ")
            for i, endpoint in enumerate(ENDPOINTS):
                name, url = endpoint['name'], endpoint['url']
                print(f"endpoint: {name} ({url}) … ", end="")

                endpoint_res = session.get(f"{API_BASE_URL}{url}")

                if not endpoint_res.ok:
                    print(f"failed to fetch items from endpoint: {name} ({url})")
                    print("error occurred, exiting … ")
                    return -1

                print(f"done. inserting into db … ", end="")

                endpoint_items = [(
                    None,
                    stored_item["id"],
                    stored_item["count"],
                    stored_item["charges"] if "charges" in stored_item else None,
                    stored_item["skin"] if "skin" in stored_item else None,
                    stored_item["category"] if "category" in stored_item else None,
                    stored_item["binding"] if "binding" in stored_item else None,
                    url
                ) for stored_item in endpoint_res.json() if stored_item is not None]

                cursor = conn.cursor()

                cursor.executemany(INSERT_TABLE_STORED_ITEMS, endpoint_items)

                cursor.close()

                print("done.")

            conn.commit()

            print("done fetching stored items.")
    except sqlite3.Error:
        traceback.print_exc()
        print("error occurred, exiting … ")
        return -1

    return 0


def search(query: str):
    try:
        with sqlite3.connect(database=f"stored_items.db") as conn:
            conn.executescript(ATTACH_DB)

            cursor = conn.cursor()

            results = cursor.execute(QUERY_STORED_ITEMS.format(query=query)).fetchall()

            print(results)

            cursor.close()
    except sqlite3.Error:
        traceback.print_exc()
        print("error occurred, exiting … ")
        return -1
    return 0


if __name__ == '__main__':
    ret = fetch_all_items()
    if ret != 0:
        sys.exit(ret)

    while True:
        ret = search(input("enter search term:"))

        if ret != 0:
            sys.exit(ret)
