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

	std::map<std::string, ItemTexture*> loadingTex;
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
	finder->Render();
}

static void tex_cb(const char* aId, Texture* aTex) {
	if (loadingTex.contains(aId)) {
		auto& savedtex = loadingTex.at(aId);

		savedtex->shader = aTex->Resource;
		savedtex->height = aTex->Width;
		savedtex->height = aTex->Height;
	}
}

static void load_texture(const char* identifier, const char* url, ItemTexture*& out_texture) {
	std::string path = std::string(url).substr(RENDER_HOST_BASE.length());

	auto instaget_tex = NEXUS->GetTextureOrCreateFromURL(
		identifier,
		RENDER_HOST_BASE.c_str(),
		path.c_str()
	);

	if (instaget_tex != nullptr) {
		out_texture->shader = instaget_tex->Resource;
		out_texture->width = instaget_tex->Width;
		out_texture->height = instaget_tex->Height;
	} else {
		out_texture->shader = freg_tex->Resource;
		out_texture->width = freg_tex->Width;
		out_texture->height = freg_tex->Height;

		loadingTex.emplace(identifier, out_texture);

		NEXUS->LoadTextureFromURL(
			identifier,
			RENDER_HOST_BASE.c_str(),
			path.c_str(),
			tex_cb
		);
	}
}

static void FREG_CALLBACK(const char* aIdentifier, Texture* aTexture) {
	freg_tex = new Texture();
	freg_tex->Resource = aTexture->Resource;
	freg_tex->Width = aTexture->Width;
	freg_tex->Height = aTexture->Height;
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

	finder = new Finder("AriKOnFire.2581", addon_dir);

	finder->InitImGui(NEXUS->ImguiContext, NEXUS->ImguiMalloc, NEXUS->ImguiFree);
	finder->SetTextureLoader(load_texture);

	NEXUS->LoadTextureFromResource(
		"freg",
		FREG_PNG,
		ADDON_MODULE,
		FREG_CALLBACK
	);
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
