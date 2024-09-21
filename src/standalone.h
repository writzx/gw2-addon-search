#pragma once
#include "resource.h"
#include "Finder.h"

#include "global.h"

typedef void (*LOAD_TEXTURE_RAW) (int resId, HMODULE aModule, void*& texture, int& width, int& height);

namespace {
	Finder* finder;
	LOAD_TEXTURE_RAW tex_loader = nullptr;
}

extern "C" __declspec(dllexport) void AddonLoadStandalone(void* ctxt, void* imgui_malloc, void* imgui_free, void* texture_loader, const char* config_dir);
extern "C" __declspec(dllexport) void AddonRenderStandalone();
