#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <filesystem>

#include "nexus/Nexus.h"
#include "imgui/imgui.h"

#include "ItemStore.h"
#include "standalone.h"
#include "helper.h"

AddonDefinition AddonDef = {};
HMODULE ADDON_MODULE = nullptr;
AddonAPI* NEXUS = nullptr;

std::string statusText = "NO DATA";
std::string searchResults = "";
bool isBusy = false;

ItemStore* store = nullptr;
APIClient* client = nullptr;

char InputBuf[256];

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
) {
	switch (ul_reason_for_call) {
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

static void search(std::string query) {
	NEXUS->Log(ELogLevel_INFO, "SEARCH", query.c_str());

	std::vector<std::string> results = store->search(query);

	searchResults = "";

	for (auto& result : results) {
		searchResults += result + "\n";
	}
}

static void AddonRender() {
	ImGuiIO& io = ImGui::GetIO();

	if (ImGui::Begin("Search")) {
		auto btn = ImGui::Button("Refresh");

		if (btn && !isBusy) {
			auto t1 = std::thread(
				[]() {
					isBusy = true;

					try {
						store->refresh();

						statusText = std::format("Last refresh: {}", helper::current_millis());
					} catch (...) {
						statusText = "EXCEPTION OCCURRED";
					}
					isBusy = false;
				}
			);
			t1.detach();
		}

		ImGui::Text(statusText.c_str());

		if (ImGui::InputText("Search", InputBuf, IM_ARRAYSIZE(InputBuf), ImGuiInputTextFlags_EnterReturnsTrue)) {
			char* s = InputBuf;
			helper::str_trim(s);

			search(s);
		}
		ImGui::SetItemDefaultFocus();

		if (ImGui::Button("Search") && !isBusy) {
			char* s = InputBuf;
			helper::str_trim(s);

			search(s);
		}

		ImGui::Text(searchResults.c_str());
	}
	ImGui::End();
}

static void AddonLoad(AddonAPI* api) {
	// save nexus for use later
	NEXUS = api;

	// init imgui
	ImGui::SetCurrentContext((ImGuiContext*) NEXUS->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))NEXUS->ImguiMalloc, (void(*)(void*, void*))NEXUS->ImguiFree);

	// register addon renderer
	NEXUS->RegisterRender(ERenderType_Render, AddonRender);

	auto addon_dir = NEXUS->GetAddonDirectory("Search");

	if (!std::filesystem::is_directory(addon_dir)) {
		std::filesystem::create_directory(addon_dir);
	}

	client = new APIClient();
	store = new ItemStore(*client, addon_dir);

	memset(InputBuf, 0, sizeof(InputBuf));
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
