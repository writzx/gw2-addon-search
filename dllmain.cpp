
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

#include "include/nexus/Nexus.h"
#include "include/imgui/imgui.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "include/httplib.h"

AddonDefinition AddonDef = {};
HMODULE ADDON_MODULE = nullptr;
AddonAPI* NEXUS = nullptr;

const std::string API_HOSTNAME = "https://api.guildwars2.com";
const std::string BANK_ENDPOINT = "/v2/account/bank";
const httplib::Headers HEADERS = {
  { "Authorization", "Bearer 84D75703-75B3-1A49-AB5A-D48B60AADBD36C8F4DF7-7858-4CFF-9258-5D985F191728" }
};

static std::string textToShow = "NO DATA";
static bool isBusy = false;


BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ADDON_MODULE = hModule;
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

static void AddonRender() {
	ImGuiIO& io = ImGui::GetIO();

	if (ImGui::Begin("Search"))
	{
		auto btn = ImGui::Button("Refresh");

		if (btn && !isBusy) {
			auto t1 = std::thread([]() {
				isBusy = true;
				httplib::Client cli(API_HOSTNAME);

				if (auto res = cli.Get(BANK_ENDPOINT, HEADERS)) {
					if (res->status == 200) {
						textToShow = "Response Data: " + res->body;

						//NEXUS->Log(ELogLevel_INFO, "API Success", res->body.c_str());
					}
				}
				else {
					auto err = res.error();
					textToShow = "HTTP error: " + httplib::to_string(err);

					//NEXUS->Log(ELogLevel_CRITICAL, "API Error", httplib::to_string(err).c_str());
				}
				isBusy = false;
				}
			);
			t1.detach();
		}

		ImGui::Text(textToShow.c_str());
	}
	ImGui::End();
}

static void AddonLoad(AddonAPI* api) {
	// save nexus for use later
	NEXUS = api;

	// init imgui
	ImGui::SetCurrentContext((ImGuiContext*)NEXUS->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))NEXUS->ImguiMalloc, (void(*)(void*, void*))NEXUS->ImguiFree);

	// register addon renderer
	NEXUS->RegisterRender(ERenderType_Render, AddonRender);
}

static void AddonUnload() {
	// deregister addon renderer
	NEXUS->DeregisterRender(AddonRender);
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef() {
	AddonDef.Signature = -1337;
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Search";
	AddonDef.Version.Major = 0;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "ArikOnFire.2581";
	AddonDef.Description = "Search for items in your account, save search queries etc.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	/* not necessary if hosted on Raidcore, but shown anyway for the example also useful as a backup resource */
	//AddonDef.Provider = EUpdateProvider_GitHub;
	//AddonDef.UpdateLink = "https://github.com/writzx/gw2-addon-Search";

	return &AddonDef;
}