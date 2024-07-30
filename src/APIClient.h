#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

using namespace std;

const string API_HOST_BASE = "https://api.guildwars2.com";
const string API_VERSION = "v2";
const map<string, string> ENDPOINTS = {
	{
		"Bank",
		"/account/bank"
	},
	{
		"Shared Inventory",
		"/account/inventory"
	},
	{
		 "Materials",
		"/account/materials"
	}
};

class APIClient {
private:
	httplib::Client client;
	string apiKey;
public:
	APIClient():
		client(API_HOST_BASE),
		apiKey("84D75703-75B3-1A49-AB5A-D48B60AADBD36C8F4DF7-7858-4CFF-9258-5D985F191728") {
		this->client.set_bearer_token_auth(this->apiKey);
	}

	string fetch(string endpoint_name);
};

