#pragma once
#include "imgui\imgui.h"

extern "C" __declspec(dllexport) void AddonLoadStandalone(ImGuiContext* ctxt, void* imgui_malloc, void* imgui_free, const char* config_dir);
extern "C" __declspec(dllexport) void AddonRenderStandalone();
