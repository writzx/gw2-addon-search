#include <filesystem>
#include <format>

#include "standalone.h"

std::map<std::string, void*> loaded_textures;

static void* LoadTexture(const char* identifier, const char* url) {
	if (loaded_textures.contains("freg")) {
		return loaded_textures.at("freg");
	}

	if (!tex_loader) {
		throw "texture loader is null";
	}

	void* shader = new void* ();
	int width = 0, height = 0;

	tex_loader(FREG_PNG, ADDON_MODULE, shader, width, height);

	loaded_textures.emplace("freg", shader);

	return shader;
}

static void* LoadTexture(const char* identifier, int resId) {
	if (loaded_textures.contains(identifier)) {
		return loaded_textures.at(identifier);
	}

	if (!tex_loader) {
		throw "texture loader is null";
	}
	void* shader = new void* ();
	int width = 0, height = 0;

	tex_loader(resId, ADDON_MODULE, shader, width, height);

	loaded_textures.emplace(identifier, shader);

	return shader;
}

void AddonLoadStandalone(void* ctxt, void* imgui_malloc, void* imgui_free, void* texture_loader, const char* config_dir) {
	const char* account_id = "AriK.3481";

	if (!std::filesystem::exists(std::format("{0}\\{1}", config_dir, account_id)) || !std::filesystem::is_directory(std::format("{0}\\{1}", config_dir, account_id))) {
		std::filesystem::create_directories(std::format("{0}\\{1}", config_dir, account_id));
	}

	finder = new Finder(account_id, config_dir);

	finder->InitImGui(ctxt, imgui_malloc, imgui_free);

	tex_loader = (LOAD_TEXTURE_RAW) texture_loader;

	finder->SetRemoteTextureLoader(LoadTexture);
	finder->SetResourceTextureLoader(LoadTexture);
}

void AddonRenderStandalone() {
	finder->Render();
}