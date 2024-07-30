#pragma once
#include <iostream>
#include <format>

using namespace std;

const string ITEM_STORE_TABLE_NAME = "item_store";

const string CREATE_TABLE_ITEM_STORE = format(
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


const string INSERT_TABLE_ITEM_STORE = format(
	"insert into {0}\n"
	"	(id, item_id, count, charges, skin, category, binding, endpoint)\n"
	"values (? , ? , ? , ? , ? , ? , ? , ?);",
	ITEM_STORE_TABLE_NAME
);


const string ATTACH_DB =
"attach '{0}' as items_db;";
// attach 'item_store.db' as item_store_db;

const string QUERY_ITEM_STORE = format(
	"select * from {0} sdb\n"
	"	inner join\n"
	"items_db.items_en idb\n"
	"	on idb.id = sdb.item_id\n"
	"where idb.name like ",
	ITEM_STORE_TABLE_NAME
) + "'%{0}%';";
