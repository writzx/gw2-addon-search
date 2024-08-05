#include <filesystem>
#include <format>

#include "standalone.h"
#include "ItemStore.h"

#include "helper.h"

std::string statusText_standalone = "NO DATA";
std::string searchResults_standalone = "";
bool isBusy_standalone = false;

ItemStore* store_standalone = nullptr;
APIClient* client_standalone = nullptr;

char InputBuf_standalone[256];

void AddonLoadStandalone(ImGuiContext* ctxt, void* imgui_malloc, void* imgui_free, const char* config_dir) {
	ImGui::SetCurrentContext(ctxt);
	ImGui::SetAllocatorFunctions((void* (*)(size_t, void*))imgui_malloc, (void(*)(void*, void*))imgui_free);

	if (!std::filesystem::is_directory(config_dir)) {
		std::filesystem::create_directory(config_dir);
	}

	client_standalone = new APIClient();
	store_standalone = new ItemStore(*client_standalone, config_dir);

	memset(InputBuf_standalone, 0, sizeof(InputBuf_standalone));
}

static void search(std::string query) {
	std::vector<std::string> results = store_standalone->search(query);

	searchResults_standalone = "";

	for (auto& result : results) {
		searchResults_standalone += result + "\n";
	}
}

void AddonRenderStandalone() {
	ImGuiIO& io = ImGui::GetIO();

	if (ImGui::Begin("Search")) {
		auto btn = ImGui::Button("Refresh");

		if (btn && !isBusy_standalone) {
			auto t1 = std::thread(
				[]() {
					isBusy_standalone = true;

					//try {
						store_standalone->refresh();

						statusText_standalone = std::format("Last refresh: {}", helper::current_millis());
					//} catch (...) {
					//	statusText_standalone = "EXCEPTION OCCURRED";
					//}
					isBusy_standalone = false;
				}
			);
			t1.detach();
		}

		ImGui::Text(statusText_standalone.c_str());

		if (ImGui::InputText("Search", InputBuf_standalone, IM_ARRAYSIZE(InputBuf_standalone), ImGuiInputTextFlags_EnterReturnsTrue)) {
			char* s = InputBuf_standalone;
			helper::str_trim(s);

			search(s);
		}
		ImGui::SetItemDefaultFocus();

		if (ImGui::Button("Search") && !isBusy_standalone) {
			char* s = InputBuf_standalone;
			helper::str_trim(s);

			search(s);
		}

		ImGui::Text(searchResults_standalone.c_str());
	}
	ImGui::End();
}