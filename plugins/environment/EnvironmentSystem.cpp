#include <filesystem>
#include <fstream>
#include "helpers/pluginHelper.hpp"

#include "Export.hpp"
#include "kengine.hpp"

#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "functions/Execute.hpp"
#include "meta/LoadFromJSON.hpp"

#include "helpers/jsonHelper.hpp"

#include "imgui.h"
#include "helpers/imfilebrowser.h"

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			loadScene("resources/default_scene.json");

			entities += [](Entity & e) noexcept {
				static ImGui::FileBrowser dialog;
				dialog.SetTitle("Load scene");
				dialog.SetTypeFilters({ ".json" });

				e += ImGuiMainMenuBarItemComponent{ "File", "Load scene", []() noexcept {
					if (ImGui::MenuItem("Load scene"))
						dialog.Open();
				} };

				e += functions::Execute{ [](float deltaTime) noexcept {
					dialog.Display();

					if (dialog.HasSelected()) {
						loadScene(dialog.GetSelected().string().c_str());
						dialog.ClearSelected();
					}
				} };
			};
		}

		static void loadScene(const char * path) noexcept {
			static std::vector<EntityID> toRemove;

			for (const auto id : toRemove)
				entities -= id;
			toRemove.clear();

			std::ifstream f(path);
			if (!f)
				return;
			const auto json = putils::json::parse(f);

			for (const auto & jsonEntity : json) {
				const auto e = jsonHelper::createEntity(jsonEntity);
				toRemove.push_back(e.id);
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}