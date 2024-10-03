#pragma once

const std::string ITEM_STORE_TABLE_NAME = "item_store";
const std::string ITEM_DB_TABLE_NAME = "items";
const std::string ENDPOINTS_TABLE_NAME = "endpoints";

const std::string ITEM_ID_COLUMN = "idb_item_id";
const std::string ITEM_NAME_COLUMN = "idb_item_name";
const std::string ITEM_DESC_COLUMN = "idb_item_description";
const std::string ITEM_LEVEL_COLUMN = "idb_item_level";
const std::string ITEM_VENDOR_VALUE_COLUMN = "idb_item_vendor_value";
const std::string ITEM_RARITY_COLUMN = "idb_item_rarity";
const std::string ITEM_TYPE_COLUMN = "idb_item_type";
const std::string ITEM_SUBTYPE_COLUMN = "idb_item_subtype";
const std::string ITEM_WEIGHT_COLUMN = "idb_item_weight";
const std::string ITEM_CHAT_LINK_COLUMN = "idb_item_chat_link";
const std::string ITEM_ICON_COLUMN = "idb_item_icon";

const std::string ITEM_STORE_ID_COLUMN = "sdb_item_store_id";
const std::string ITEM_COUNT_COLUMN = "sdb_item_count";
const std::string ITEM_CHARGES_COLUMN = "sdb_item_charges";
const std::string ITEM_SKIN_COLUMN = "sdb_item_skin";
const std::string ITEM_CATEGORY_COLUMN = "sdb_item_category";
const std::string ITEM_BINDING_COLUMN = "sdb_item_binding";

const std::string ENDPOINT_ID_COLUMN = "edb_ep_id";
const std::string ENDPOINT_LABEL_COLUMN = "edb_ep_label";
const std::string ENDPOINT_UPDATED_COLUMN = "edb_ep_updated";

const std::string CREATE_TABLE_ITEM_STORE = std::format(
    "drop table if exists {0};\n"
    "create table {0} (\n"
    "    id              integer     primary key,\n"
    "    item_id         integer     not null,\n"
    "    count           int         not null,\n"

    "    charges         int,\n"

    "    skin            integer,\n"
    "    category        integer,\n"
    "    binding         text,\n"

    "    endpoint        text,\n"
    "    foreign key     (endpoint) REFERENCES {1}(id)"
    ");",
    ITEM_STORE_TABLE_NAME,
    ENDPOINTS_TABLE_NAME
);

const std::string CREATE_ENDPOINTS_TABLE = std::format(
    "create table if not exists {0} (\n"
    "    id			text		primary key,\n"
    "    label		text		not null,\n"
    "    updated	text\n"
    ");",
    ENDPOINTS_TABLE_NAME
);

const std::string NEW_ITEM_TRIGGER = std::format(
    "create trigger if not exists update_endpoints\n"
    // maybe also write trigger for update operation, but I don't see it being needed
    "after insert on {0}\n"
    "for each row\n"
    "begin\n"
    "    update {1}\n"
    "    set updated = datetime('now')\n"
    "    where id = new.endpoint;\n"
    // this insert is just a fallback in case we didn't create the endpoints
    "    insert into {1}(id, label, updated)\n"
    "        select new.endpoint, new.endpoint, datetime('now')\n"
    "            where not exists(select 1 from {1} where id = new.endpoint);\n"
    "end;",
    ITEM_STORE_TABLE_NAME,
    ENDPOINTS_TABLE_NAME
);

const std::string INSERT_TABLE_ITEM_STORE = std::format(
    "insert into {0}\n"
    "    (id, item_id, count, charges, skin, category, binding, endpoint)\n"
    "values (? , ? , ? , ? , ? , ? , ? , ?);",
    ITEM_STORE_TABLE_NAME
);

const std::string INSERT_TABLE_ENDPOINTS = std::format(
    "insert into {0}\n"
    "    (id, label, updated)\n"
    "values (? , ? , NULL)\n"
    "    on conflict(id)\n"
    "do update set label = excluded.label;\n",
    ENDPOINTS_TABLE_NAME
);


const std::string ATTACH_DB = "attach '{0}' as items_db;";

const std::string LIST_ENDPOINTS = std::format(
    "select\n"
    "id, label, updated\n"
    "from {0};",
    ENDPOINTS_TABLE_NAME
);

const std::string QUERY_ITEM_STORE = std::format(
    "select\n"
    "    idb.id as {4},\n"
    "    idb.name as {5},\n"
    "    idb.description as {6},\n"
    "    idb.item_level as {7},\n"
    "    idb.vendor_value as {8},\n"
    "    idb.rarity as {9},\n"
    "    idb.type as {10},\n"
    "    idb.subtype as {11},\n"
    "    idb.weight as {12},\n"
    "    idb.chat_link as {13},\n"
    "    idb.icon as {14},\n"
    "    sdb.id as {15},\n"
    "    sdb.count as {16},\n"
    "    sdb.charges as {17},\n"
    "    sdb.skin as {18},\n"
    "    sdb.category as {19},\n"
    "    sdb.binding as {20},\n"
    "    edb.id as {21},\n"
    "    edb.label as {22},\n"
    "    edb.updated as {23}\n"
    "from\n"
    "    {0} sdb\n"
    "    inner join items_db.{1} idb\n"
    "        on idb.id = sdb.item_id\n"
    "    inner join {2} edb\n"
    "        on sdb.endpoint = edb.id\n"
    "where idb.name like {3};",
    ITEM_STORE_TABLE_NAME,
    ITEM_DB_TABLE_NAME,
    ENDPOINTS_TABLE_NAME,
    "'%{0}%'",
    ITEM_ID_COLUMN,
    ITEM_NAME_COLUMN,
    ITEM_DESC_COLUMN,
    ITEM_LEVEL_COLUMN,
    ITEM_VENDOR_VALUE_COLUMN,
    ITEM_RARITY_COLUMN,
    ITEM_TYPE_COLUMN,
    ITEM_SUBTYPE_COLUMN,
    ITEM_WEIGHT_COLUMN,
    ITEM_CHAT_LINK_COLUMN,
    ITEM_ICON_COLUMN,
    ITEM_STORE_ID_COLUMN,
    ITEM_COUNT_COLUMN,
    ITEM_CHARGES_COLUMN,
    ITEM_SKIN_COLUMN,
    ITEM_CATEGORY_COLUMN,
    ITEM_BINDING_COLUMN,
    ENDPOINT_ID_COLUMN,
    ENDPOINT_LABEL_COLUMN,
    ENDPOINT_UPDATED_COLUMN
);
