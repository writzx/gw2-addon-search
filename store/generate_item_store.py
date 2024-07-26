import sys
import sqlite3
import traceback
import argparse

from requests import Session
from requests.adapters import HTTPAdapter, Retry

API_BASE_URL = "https://api.guildwars2.com/v2"
BATCH_SIZE = 100

session = Session()

retries = Retry(
    total=5,
    backoff_factor=0.1,
    status_forcelist=[500, 502, 503, 504]
)

session.mount('http://', HTTPAdapter(max_retries=retries))


def batch_item_ids(all_ids, n=BATCH_SIZE, return_indices=False):
    for i in range(0, len(all_ids), n):
        if return_indices:
            yield all_ids[i:i + n], i + 1, min(i + n, len(all_ids))
        else:
            yield all_ids[i:i + n]


"""

# item instance specific
     binding
     upgrades
     item upgrades?? (include?)
     equipment?? (include?)
--
# ui only
     sort order??
     grouping??

"""

TABLE_NAME = f"items_{{lang}}"

CREATE_TABLE_ITEMS = f"""
drop table if exists {TABLE_NAME};
create table {TABLE_NAME} (
    id              integer     primary key,
    name            text        not null,
    description     text,

    item_level      int         not null,
    vendor_value    integer     not null,

    rarity          text        not null,
    type            text        not null,
    subtype         text,
    weight          text,

    chat_link       text        not null,
    icon            text
);
"""

INSERT_TABLE_ITEMS = f"""
insert into {TABLE_NAME}
(id, name, description, item_level, vendor_value, rarity, type, subtype, weight, chat_link, icon)
values (?,?,?,?,?,?,?,?,?,?,?);
"""


def main(lang: str) -> int:
    try:
        print(f"generating items db for language: {lang}.")
        with sqlite3.connect(database=f"items_{lang}.db") as conn:
            cursor = conn.cursor()

            cursor.executescript(CREATE_TABLE_ITEMS.format(lang=lang))

            conn.commit()
            cursor.close()

            print("fetching a list of items from the gw2 api … ", end="")
            items_res = session.get(f"{API_BASE_URL}/items?lang={lang}")

            if not items_res.ok:
                print("failed to fetch item ids")
                print("error occurred, exiting … ")
                return -1

            item_ids = items_res.json()

            print(f"done.")
            print(f"found {len(item_ids)} item ids.")

            for batch_ids, start, end in batch_item_ids(item_ids, return_indices=True):
                cursor = conn.cursor()
                print(f"fetching items indices: {start}-{end} [{end}/{len(item_ids)}] … ", end="")
                items = session.get(f"{API_BASE_URL}/items?ids={",".join([f'{i}' for i in batch_ids])}&lang={lang}")

                if items.status_code != 200:
                    print(f"failed to fetch item ids: {batch_ids[0]}-{batch_ids[-1]}")
                    print("error occurred, exiting … ")
                    return -1

                print(f"done. inserting into db … ", end="")

                item_tups = [(
                    item["id"],
                    item["name"],
                    item["description"] if "description" in item else None,
                    item["level"],
                    item["vendor_value"],
                    item["rarity"],
                    item["type"],
                    item["details"]["type"] if "details" in item and "type" in item["details"] else None,
                    item["details"]["weight_class"] if "details" in item
                                                       and "weight_class" in item["details"] else None,
                    item["chat_link"],
                    item["icon"] if "icon" in item else None,
                ) for item in items.json()]

                cursor.executemany(INSERT_TABLE_ITEMS.format(lang=lang), item_tups)

                cursor.close()

                print(f"done.", end="\r")

            conn.commit()

            print()
            print(f"completed generating items db for language: {lang}.")
    except sqlite3.Error:
        traceback.print_exc()
        print("error occurred, exiting … ")
        return -1

    return 0


if __name__ == '__main__':
    languages = ["en", "es", "de", "fr", "zh"]
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "-l",
        "--lang",
        choices=[*languages, "all"],
        default="all",
        nargs="*",
        help="Only fetch items for the specified languages.",
        type=str.lower  # type: ignore
    )

    args = parser.parse_args()


    def fetch_langs(langs: list[str]):
        for lang in langs:
            retval = main(lang)

            if retval != 0:
                return retval
        return 0


    if args.lang == "all":
        print("generating items db for all languages … ")

        ret = fetch_langs(languages)

        print(f"completed generating items db for all languages.")

        sys.exit(ret)
    else:
        print(f"generating items db for specified languages: {repr(args.lang)} … ")

        ret = fetch_langs(args.lang)

        print(f"completed generating items db for specified languages.")

        sys.exit(ret)
