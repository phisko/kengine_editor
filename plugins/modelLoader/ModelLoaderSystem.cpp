#include <filesystem>
#include <unordered_set>

#include <GLFW/glfw3.h>

#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/ImGuiComponent.hpp"
#include "data/GraphicsComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "data/GLFWWindowComponent.hpp"

#include "functions/Execute.hpp"

#include "helpers/pluginHelper.hpp"
#include "helpers/assertHelper.hpp"

#include "imgui.h"
#include "imfilebrowser.h"

static kengine::EntityManager * g_em;

#pragma region declarations
static void loadModel(const char * path);
static void execute(float deltaTime);
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

		e += kengine::functions::Execute{ execute };
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

static void execute(float deltaTime) {
	for (const auto & [e, window] : g_em->getEntities<kengine::GLFWWindowComponent>())
		glfwSetDropCallback(window.window, [](GLFWwindow * window, int nbFiles, const char ** files) {
			kengine_assert(*g_em, nbFiles >= 1);
			loadModel(files[0]);
		});
}