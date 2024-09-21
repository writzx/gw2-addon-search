#include <filesystem>
#include <format>

#include "standalone.h"

static void LoadTexture(const char* identifier, const char* url, ItemTexture*& out_texture) {
	if (!tex_loader) {
		throw "texture loader is null";
	}

	tex_loader(FREG_PNG, ADDON_MODULE, out_texture->shader, out_texture->width, out_texture->height);
}

void AddonLoadStandalone(void* ctxt, void* imgui_malloc, void* imgui_free, void* texture_loader, const char* config_dir) {
	finder = new Finder("AriKOnFire.2581", config_dir);

	finder->InitImGui(ctxt, imgui_malloc, imgui_free);

	tex_loader = (LOAD_TEXTURE_RAW) texture_loader;

	finder->SetTextureLoader(LoadTexture);
}

void AddonRenderStandalone() {
	finder->Render();
}