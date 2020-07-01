#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "functions/Execute.hpp"

#include "helpers/pluginHelper.hpp"
#include "helpers/sortHelper.hpp"

#include "imgui.h"

EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	em += [&](kengine::Entity & e) {
		e += kengine::functions::Execute{ [&](float deltaTime) {
			const auto entities = kengine::sortHelper::getSortedEntities<0, ImGuiMainMenuBarItemComponent>(em, [](const auto & a, const auto & b) {
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