#include <filesystem>

#include "standalone.h"
#include "Finder.h"
#include "resource.h"

namespace {
    Finder *finder;
    LOAD_TEXTURE_RAW tex_loader = nullptr;
    std::map<std::string, void *> loaded_textures;
}

static void *LoadTexture(const char */*identifier*/, const char */*url*/) {
    if (loaded_textures.contains("freg")) {
        return loaded_textures.at("freg");
    }

    if (!tex_loader) {
        throw "texture loader is null";
    }

    void *shader = new void *();
    int width = 0, height = 0;

    tex_loader(FREG_PNG, ADDON_MODULE, shader, width, height);

    loaded_textures.emplace("freg", shader);

    return shader;
}

static void *LoadTexture(const char *identifier, int resId) {
    if (loaded_textures.contains(identifier)) {
        return loaded_textures.at(identifier);
    }

    if (!tex_loader) {
        throw "texture loader is null";
    }
    void *shader = new void *();
    int width = 0, height = 0;

    tex_loader(resId, ADDON_MODULE, shader, width, height);

    loaded_textures.emplace(identifier, shader);

    return shader;
}

void AddonLoadStandalone(
    void *ctxt,
    void *imgui_malloc,
    void *imgui_free,
    void *texture_loader,
    const char *config_dir
) {
    const auto account_id = "AriKOnFire.2581";

    const auto config_path = std::filesystem::path(config_dir);

    if (!exists(config_path / account_id) || !
        is_directory(config_path / account_id)) {
        create_directories(config_path / account_id);
    }

    finder = new Finder(account_id, config_path);

    Finder::InitImGui(ctxt, imgui_malloc, imgui_free);

    finder->Toggle();

    tex_loader = static_cast<LOAD_TEXTURE_RAW>(texture_loader);

    finder->SetRemoteTextureLoader(LoadTexture);
    finder->SetResourceTextureLoader(LoadTexture);
}

void AddonRenderStandalone() {
    finder->Render();
}
