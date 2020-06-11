#include <filesystem>

#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/ImGuiComponent.hpp"
#include "data/GraphicsComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"

#include "helpers/pluginHelper.hpp"

#include "json.hpp"
#include "imgui.h"
#include "imfilebrowser.h"

kengine::EntityManager * g_em;

#pragma region declarations
static void loadModel(const char * path);
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);

	g_em = &em;

	em += [&](kengine::Entity & e) {
		static ImGui::FileBrowser dialog;
		dialog.SetTitle("Load model");

		e += ImGuiMainMenuBarItemComponent{ "File", "Load model", [] {
			dialog.Open();
		} };

		e += kengine::ImGuiComponent([] {
			dialog.Display();

			if (dialog.HasSelected()) {
				loadModel(dialog.GetSelected().string().c_str());
				dialog.ClearSelected();
			}
		});
	};
}

static void loadModel(const char * path) {
	static kengine::Entity::ID toRemove = kengine::Entity::INVALID_ID;

	if (toRemove != kengine::Entity::INVALID_ID) {
		const auto & e = g_em->getEntity(toRemove);
		if (e.has<kengine::InstanceComponent>())
			g_em->removeEntity(e.get<kengine::InstanceComponent>().model);

		g_em->removeEntity(toRemove);
	}

	*g_em += [&](kengine::Entity & e) {
		toRemove = e.id;
		e += kengine::GraphicsComponent{ path };
		e += kengine::TransformComponent{};
	};
}