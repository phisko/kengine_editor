#include <filesystem>

#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/ImGuiComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "meta/LoadFromJSON.hpp"

#include "helpers/pluginHelper.hpp"

#include "json.hpp"
#include "imgui.h"
#include "imfilebrowser.h"

kengine::EntityManager * g_em;

#pragma region declarations
static void loadScene(const char * path);
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	g_em = &em;

	loadScene("resources/default_scene.json");

	em += [&](kengine::Entity & e) {
		static ImGui::FileBrowser dialog;
		dialog.SetTitle("Load scene");
		dialog.SetTypeFilters({ ".json" });

		e += ImGuiMainMenuBarItemComponent{ "File", "Load scene", [] {
			dialog.Open();
		} };

		e += kengine::ImGuiComponent([] {
			dialog.Display();

			if (dialog.HasSelected()) {
				loadScene(dialog.GetSelected().string().c_str());
				dialog.ClearSelected();
			}
		});
	};
}

#pragma region loadScene
#pragma region declarations
static void loadEntity(kengine::Entity & e, const putils::json & json);
#pragma endregion
static void loadScene(const char * path) {
	static std::vector<kengine::Entity::ID> toRemove;

	for (const auto id : toRemove)
		g_em->removeEntity(id);
	toRemove.clear();

	std::ifstream f(path);
	if (!f)
		return;
	const auto json = putils::json::parse(f);

	for (const auto & jsonEntity : json) {
		*g_em += [&](kengine::Entity & e) {
			toRemove.push_back(e.id);
			loadEntity(e, jsonEntity);
		};
	}
}

static void loadEntity(kengine::Entity & e, const putils::json & json) {
	for (const auto & [_, loader] : g_em->getEntities<kengine::meta::LoadFromJSON>())
		loader(json, e);
}
#pragma endregion loadScene