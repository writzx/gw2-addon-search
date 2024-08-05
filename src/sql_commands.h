#pragma once
#include <iostream>

const std::string ITEM_STORE_TABLE_NAME = "item_store";
const std::string ITEM_NAME_COLUMN = "item_name";

const std::string CREATE_TABLE_ITEM_STORE = std::format(
	"drop table if exists {0};\n"
	"create table {0} (\n"
	"	id              integer     primary key,\n"
	"	item_id         integer     not null,\n"
	"	count           int         not null,\n"

	"	charges         int,\n"

	"	skin            integer,\n"
	"	category        integer,\n"
	"	binding         text,\n"

	"	endpoint        text\n"
	");",
	ITEM_STORE_TABLE_NAME
);


const std::string INSERT_TABLE_ITEM_STORE = std::format(
	"insert into {0}\n"
	"	(id, item_id, count, charges, skin, category, binding, endpoint)\n"
	"values (? , ? , ? , ? , ? , ? , ? , ?);",
	ITEM_STORE_TABLE_NAME
);


const std::string ATTACH_DB = "attach '{0}' as items_db;";
// attach 'item_store.db' as item_store_db;

const std::string QUERY_ITEM_STORE = std::format(
	"select idb.name as {0} from {1} sdb\n"
	"	inner join\n"
	"items_db.items_en idb\n"
	"	on idb.id = sdb.item_id\n"
	"where idb.name like {2}",
	ITEM_NAME_COLUMN,
	ITEM_STORE_TABLE_NAME,
	"'%{0}%';"
);
