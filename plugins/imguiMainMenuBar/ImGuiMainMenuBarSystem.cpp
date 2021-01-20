#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "functions/Execute.hpp"

#include "helpers/sortHelper.hpp"

#include "imgui.h"

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	pluginHelper::initPlugin(state);

	entities += [](Entity & e) noexcept {
		e += functions::Execute{ [](float deltaTime) noexcept {
			const auto entities = sortHelper::getSortedEntities<0, ImGuiMainMenuBarItemComponent>([](const auto & a, const auto & b) noexcept {
				const auto & first = *std::get<1>(a);
				const auto & second = *std::get<1>(b);

				const auto cmp = strcmp(first.menu.c_str(), second.menu.c_str());
				if (cmp != 0)
					return cmp < 0;

				return strcmp(first.itemName.c_str(), second.itemName.c_str()) < 0;
			});

			std::string currentMenu;
			bool currentMenuOpen = false;

			if (ImGui::BeginMainMenuBar()) {
				for (const auto & [e, item] : entities) {
					if (item->menu != currentMenu) {
						if (currentMenuOpen)
							ImGui::EndMenu();

						currentMenu = item->menu;
						currentMenuOpen = ImGui::BeginMenu(currentMenu.c_str());
					}

					if (currentMenuOpen)
						item->draw();
				}

				if (currentMenuOpen)
					ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		} };
	};
}