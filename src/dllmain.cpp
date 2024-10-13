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
    AddonAPI *NEXUS = nullptr;
    Finder *finder = nullptr;

    Texture *freg_tex = nullptr;
}

BOOL APIENTRY DllMain(
    const HMODULE hModule,
    const DWORD ul_reason_for_call,
    LPVOID /*lpReserved*/
) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            ADDON_MODULE = hModule;
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
        default:
            break;
    }
    return TRUE;
}

static void AddonSettings() {
    if (finder == nullptr) {
        return;
    }

    finder->RenderSettingsView();
}

static void AddonRender() {
    if (finder == nullptr) {
        return;
    }

    finder->Render();
}

static void *load_texture(const char *identifier, const char *url) {
    const std::string path = std::string(url).substr(RENDER_HOST_BASE.length());

    const auto instaget_tex = NEXUS->GetTextureOrCreateFromURL(
        identifier,
        RENDER_HOST_BASE.c_str(),
        path.c_str()
    );

    if (instaget_tex != nullptr) {
        return instaget_tex->Resource;
    }

    NEXUS->LoadTextureFromURL(
        identifier,
        RENDER_HOST_BASE.c_str(),
        path.c_str(),
        [](const char *, Texture *) {
        }
    );

    return freg_tex->Resource;
}

static void *load_texture(const char *identifier, const int resId) {
    const auto instaget_tex = NEXUS->GetTextureOrCreateFromResource(
        identifier,
        resId,
        ADDON_MODULE
    );

    if (instaget_tex != nullptr) {
        return instaget_tex->Resource;
    }

    NEXUS->LoadTextureFromResource(
        identifier,
        resId,
        ADDON_MODULE,
        [](const char *, Texture *) {
        }
    );

    return freg_tex->Resource;
}

static void LoadFregCallback(const char */*aIdentifier*/, Texture *aTexture) {
    freg_tex = new Texture();
    freg_tex->Resource = aTexture->Resource;
    freg_tex->Width = aTexture->Width;
    freg_tex->Height = aTexture->Height;
}

static void FixOldAddonDir() noexcept {
    if (const std::filesystem::path addon_dir(NEXUS->GetAddonDirectory("Search"));
        exists(addon_dir / ITEMS_DB_FILENAME)) {
        // confirmed that we have an old addon dir
        if (exists(addon_dir / "search.json")) {
            // try to rename old config
            try {
                std::filesystem::rename(addon_dir / "search.json", addon_dir / CONFIG_JSON_FILENAME);
            } catch (...) {
            }
        }

        // try to rename addon dir
        try {
            std::filesystem::rename(addon_dir, addon_dir.parent_path() / ADDON_NAME);
        } catch (...) {
        }
    }
}

static void ToggleFinder(const char */*aIdentifier*/, bool aIsRelease) {
    if (finder == nullptr) {
        return;
    }

    if (aIsRelease) {
        finder->Toggle();
    }
}

static void HandleAccountName(void *account_name) {
    if (!account_name) {
        return;
    }

    const std::string raw_id = static_cast<const char *>(account_name);
    const std::filesystem::path addon_dir(NEXUS->GetAddonDirectory(ADDON_NAME.c_str()));

    const auto id = raw_id.substr(raw_id.find_first_not_of(':'), raw_id.find_last_not_of(':'));

    if (const std::filesystem::path store_path = addon_dir / id; !exists(store_path) || !is_directory(store_path)) {
        create_directories(store_path);
    }

    finder = new Finder(id, addon_dir);

    Finder::InitImGui(NEXUS->ImguiContext, NEXUS->ImguiMalloc, NEXUS->ImguiFree);

    // register addon renderer
    NEXUS->RegisterRender(ERenderType_Render, AddonRender);
    NEXUS->RegisterRender(ERenderType_OptionsRender, AddonSettings);

    NEXUS->AddShortcut("ToggleFinder", "freg_icon", "freg_icon_hover", "ToggleFinder", "Show Finder Window");
    NEXUS->RegisterKeybindWithString("ToggleFinder", ToggleFinder, "Ctrl+F");

    finder->SetRemoteTextureLoader(load_texture);
    finder->SetResourceTextureLoader(load_texture);
}

static void AddonLoad(AddonAPI *api) {
    // save nexus for use later
    NEXUS = api;

    FixOldAddonDir();

    // init imgui
    ImGui::SetCurrentContext(static_cast<ImGuiContext *>(NEXUS->ImguiContext));
    ImGui::SetAllocatorFunctions(
        static_cast<void* (*)(size_t, void *)>(NEXUS->ImguiMalloc),
        static_cast<void(*)(void *, void *)>(NEXUS->ImguiFree)
    );

    NEXUS->SubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);
    NEXUS->RaiseEvent("EV_REQUEST_ACCOUNT_NAME", nullptr);

    NEXUS->LoadTextureFromResource(
        "freg",
        FREG_PNG,
        ADDON_MODULE,
        LoadFregCallback
    );

    NEXUS->LoadTextureFromResource(
        "freg_icon",
        FREG_ICON_PNG,
        ADDON_MODULE,
        [](const char *, Texture *) {
        }
    );

    NEXUS->LoadTextureFromResource(
        "freg_icon_hover",
        FREG_ICON_HOVER_PNG,
        ADDON_MODULE,
        [](const char *, Texture *) {
        }
    );
}

static void AddonUnload() {
    NEXUS->UnsubscribeEvent("EV_ACCOUNT_NAME", HandleAccountName);

    if (finder != nullptr) {
        // deregister addon renderer
        NEXUS->DeregisterRender(AddonRender);
        NEXUS->DeregisterRender(AddonSettings);

        NEXUS->RemoveShortcut("ToggleFinder");
        NEXUS->DeregisterKeybind("ToggleFinder");
    }
}

extern "C" __declspec(dllexport) AddonDefinition *GetAddonDef() {
    AddonDef.Signature = -1337;
    AddonDef.APIVersion = NEXUS_API_VERSION;
    AddonDef.Name = ADDON_NAME.c_str();
    AddonDef.Version.Major = 0;
    AddonDef.Version.Minor = 0;
    AddonDef.Version.Build = 3;
    AddonDef.Version.Revision = 0;
    AddonDef.Author = "ArikOnFire.2581";
    AddonDef.Description = "Search for items in your account, save search queries etc.";
    AddonDef.Load = AddonLoad;
    AddonDef.Unload = AddonUnload;
    AddonDef.Flags = EAddonFlags_None;

    AddonDef.Provider = EUpdateProvider_GitHub;
    AddonDef.UpdateLink = "https://github.com/writzx/gw2-addon-search";

    return &AddonDef;
}
