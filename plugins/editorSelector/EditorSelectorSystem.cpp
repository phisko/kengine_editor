#include <fstream>
#include <unordered_set>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/EditorComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"

#include "functions/OnTerminate.hpp"

#include "helpers/assertHelper.hpp"
#include "imgui.h"

using namespace kengine;

static constexpr auto RECENT_FILE = "recentEditor.txt";

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += ImGuiMainMenuBarItemComponent{ "Edit", "All", drawImGui };
				e += functions::OnTerminate{ onTerminate };
			};
		}

		static void drawImGui() noexcept {
			static bool first = true;
			if (first) {
				reloadRecent();
				first = false;
			}

			for (const auto & [e, editor] : entities.with<EditorComponent>())
				ImGui::MenuItem(editor.name.c_str(), nullptr, editor.active);
		}

		static void reloadRecent() noexcept {
			std::ifstream f(RECENT_FILE);
			if (!f) {
				kengine_assert_failed("Failed to open '", RECENT_FILE, "'");
				return;
			}

			std::unordered_set<std::string> lastOpened;
			for (std::string line; std::getline(f, line);)
				lastOpened.insert(line);

			for (const auto & [e, editor] : entities.with<EditorComponent>())
				*editor.active = lastOpened.contains(editor.name);
		}

		static void onTerminate() noexcept {
			std::ofstream f(RECENT_FILE);
			if (!f) {
				kengine_assert_failed("Failed to open '", RECENT_FILE, "' for writing");
				return;
			}

			for (const auto & [e, editor] : entities.with<EditorComponent>())
				if (*editor.active) {
					f << editor.name;
					return;
				}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
