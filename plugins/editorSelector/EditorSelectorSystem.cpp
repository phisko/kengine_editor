#include <fstream>
#include <unordered_set>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/EditorComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/OnTerminate.hpp"

#include "helpers/assertHelper.hpp"
#include "helpers/sortHelper.hpp"
#include "imgui.h"

using namespace kengine;

static constexpr auto RECENT_FILE = "recentEditor.txt";
static EntityID g_id = INVALID_ID;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				g_id = e.id;

				e += ImGuiMainMenuBarItemComponent{ "Edit", "All", drawImGui };
				e += functions::Execute{ execute };
				e += functions::OnTerminate{ onTerminate };
			};
		}

		static void execute(float deltaTime) noexcept {
			reloadRecent();
			entities[g_id].detach<functions::Execute>();
		}

		static void drawImGui() noexcept {
			const auto sortedEntities = sortHelper::getSortedEntities<EditorComponent>([](const auto & lhsTuple, const auto & rhsTuple) {
				const auto lhs = std::get<1>(lhsTuple);
				const auto rhs = std::get<1>(rhsTuple);
				return strcmp(lhs->name.c_str(), rhs->name.c_str()) < 0;
			});

			for (const auto & [e, editor] : sortedEntities) {
				if (*editor->active) {
					ImGui::MenuItem(editor->name.c_str(), nullptr, editor->active);
					*editor->active = true;
					continue;
				}

				if (!ImGui::MenuItem(editor->name.c_str(), nullptr, editor->active))
					continue;

				for (const auto & [other, otherEditor] : entities.with<EditorComponent>())
					if (other.id != e.id)
						*otherEditor.active = false;
				return;
			}
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
