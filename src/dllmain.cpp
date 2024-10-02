#include <string>
#include <filesystem>

#include "global.h"
#include "resource.h"

#include "nexus/Nexus.h"
#include "imgui/imgui.h"

#include "Finder.h"

HMODULE ADDON_MODULE = nullptr;

namespace {
	AddonDefinition AddonDef = {};
	AddonAPI* NEXUS = nullptr;
	Finder* finder = nullptr;

	Texture* freg_tex = nullptr;
}

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

static void AddonRender() {
	if (finder == nullptr) {
		return;
	}

	finder->Render();
}

static void* load_texture(const char* identifier, const char* url) {
	std::string path = std::string(url).substr(RENDER_HOST_BASE.length());

	auto instaget_tex = NEXUS->GetTextureOrCreateFromURL(
		identifier,
		RENDER_HOST_BASE.c_str(),
		path.c_str()
	);

	if (instaget_tex != nullptr) {
		return instaget_tex->Resource;
	} else {
		NEXUS->LoadTextureFromURL(
			identifier,
			RENDER_HOST_BASE.c_str(),
			path.c_str(),
			[](const char*, Texture*) {}
		);

		return freg_tex->Resource;
	}
}

static void* load_texture(const char* identifier, int resId) {
	auto instaget_tex = NEXUS->GetTextureOrCreateFromResource(
		identifier,
		resId,
		ADDON_MODULE
	);

	if (instaget_tex != nullptr) {
		return instaget_tex->Resource;
	} else {
		NEXUS->LoadTextureFromResource(
			identifier,
			resId,
			ADDON_MODULE,
			[](const char*, Texture*) {}
		);

		return freg_tex->Resource;
	}
}

static void LoadFregCallback(const char* aIdentifier, Texture* aTexture) {
	freg_tex = new Texture();
	freg_tex->Resource = aTexture->Resource;
	freg_tex->Width = aTexture->Width;
	freg_tex->Height = aTexture->Height;
}

static void HandleAccountName(void* account_name) {
	if (!account_name) {
		return;
	}

	std::string raw_id = static_cast<const char*>(account_name);
	std::filesystem::path addon_dir(NEXUS->GetAddonDirectory("Search"));

	auto id = raw_id.substr(raw_id.find_first_not_of(":"), raw_id.find_last_not_of(":"));

	std::filesystem::path store_path = addon_dir / id;

	if (!std::filesystem::exists(store_path) || !std::filesystem::is_directory(store_path)) {
		std::filesystem::create_directories(store_path);
	}

	finder = new Finder(id, addon_dir);

	finder->InitImGui(NEXUS->ImguiContext, NEXUS->ImguiMalloc, NEXUS->ImguiFree);

	finder->SetRemoteTextureLoader(load_texture);
	finder->SetResourceTextureLoader(load_texture);
}

static void ToggleFinder(const char* aIdentifier, bool aIsRelease) {
	if (finder == nullptr) {
		return;
	}

	finder->Toggle();
}

static void ToggleFinder() {
	if (finder == nullptr) {
		return;
	}

	finder->Toggle();
}

static void AddonLoad(AddonAPI* api) {
	// save nexus for use later
	NEXUS = api;

	// init imgui
	ImGui::SetCurrentContext((ImGuiContext*) NEXUS->ImguiContext);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))NEXUS->ImguiMalloc, (void(*)(void*, void*))NEXUS->ImguiFree);

	// register addon renderer
	NEXUS->RegisterRender(ERenderType_Render, AddonRender);

	NEXUS->SubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);
	NEXUS->RaiseEvent("EV_REQUEST_ACCOUNT_NAME", NULL);

	NEXUS->LoadTextureFromResource(
		"freg",
		FREG_PNG,
		ADDON_MODULE,
		LoadFregCallback
	);

	NEXUS->AddShortcut("ToggleFinder", "freg", "freg", "ToggleFinder", "Show Finder Window");
	NEXUS->RegisterKeybindWithString("ToggleFinder", ToggleFinder, "Ctrl+F");
}

static void AddonUnload() {
	// deregister addon renderer
	NEXUS->DeregisterRender(AddonRender);

	NEXUS->UnsubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);

	NEXUS->RemoveShortcut("ToggleFinder");
	NEXUS->DeregisterKeybind("ToggleFinder");
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef() {
	AddonDef.Signature = -1337;
	AddonDef.APIVersion = NEXUS_API_VERSION;
	AddonDef.Name = "Finder";
	AddonDef.Version.Major = 0;
	AddonDef.Version.Minor = 0;
	AddonDef.Version.Build = 1;
	AddonDef.Version.Revision = 0;
	AddonDef.Author = "ArikOnFire.2581";
	AddonDef.Description = "Search for items in your account, save search queries etc.";
	AddonDef.Load = AddonLoad;
	AddonDef.Unload = AddonUnload;
	AddonDef.Flags = EAddonFlags_None;

	AddonDef.Provider = EUpdateProvider_GitHub;
	AddonDef.UpdateLink = "https://github.com/writzx/gw2-addon-Search";

	return &AddonDef;
}
