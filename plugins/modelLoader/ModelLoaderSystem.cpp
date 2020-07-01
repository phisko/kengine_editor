#include <filesystem>
#include <unordered_set>
#include <vector>

#include <GLFW/glfw3.h>

#include "Export.hpp"
#include "EntityManager.hpp"

#include "data/GraphicsComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "data/GLFWWindowComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/OnTerminate.hpp"

#include "helpers/pluginHelper.hpp"
#include "helpers/assertHelper.hpp"

#include "imgui.h"
#include "imfilebrowser.h"

static constexpr auto RECENT_FILE = "recentModels.txt";

static kengine::EntityManager * g_em;
static std::list<std::string> g_recentItems;
static ImGui::FileBrowser g_dialog;

#pragma region declarations
static void loadRecentItems();
static void saveRecentItems();
static void loadModel(const char * path);
static void execute(float deltaTime);
#pragma endregion
EXPORT void loadKenginePlugin(kengine::EntityManager & em) {
	kengine::pluginHelper::initPlugin(em);
	loadRecentItems();

	g_em = &em;


	em += [&](kengine::Entity & e) {
		g_dialog.SetTitle("Load model");

		e += kengine::functions::Execute{ execute };
		e += kengine::functions::OnTerminate{ saveRecentItems };
	};


	em += [](kengine::Entity & e) {
		e += ImGuiMainMenuBarItemComponent{ "File", "Load model", [] {
			if (ImGui::MenuItem("Load model"))
				g_dialog.Open();
		} };
	};

	em += [](kengine::Entity & e) {
		e += ImGuiMainMenuBarItemComponent{ "File", "Recent models", [] {
			if (ImGui::BeginMenu("Recent models")) {
				for (const auto & f : g_recentItems)
					if (ImGui::MenuItem(f.c_str()))
						loadModel(f.c_str());
				ImGui::EndMenu();
			}
		} };
	};
}

static void loadRecentItems() {
	std::ifstream f(RECENT_FILE);

	if (!f)
		return;

	for (std::string s; std::getline(f, s);)
		g_recentItems.push_back(s);
}

static void saveRecentItems() {
	std::ofstream f(RECENT_FILE, std::ofstream::trunc);

	if (!f)
		return;

	for (const auto & s : g_recentItems)
		f << s << std::endl;
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

	const auto it = std::find(g_recentItems.begin(), g_recentItems.end(), path);
	if (it != g_recentItems.end())
		g_recentItems.splice(g_recentItems.begin(), g_recentItems, it);
	else
		g_recentItems.push_front(path);
}

static void execute(float deltaTime) {
	g_dialog.Display();

	if (g_dialog.HasSelected()) {
		loadModel(g_dialog.GetSelected().string().c_str());
		g_dialog.ClearSelected();
	}

	for (const auto & [e, window] : g_em->getEntities<kengine::GLFWWindowComponent>()) {
		glfwSetDropCallback(window.window, [](GLFWwindow * window, int nbFiles, const char ** files) {
			kengine_assert(*g_em, nbFiles >= 1);
			loadModel(files[0]);
		});
	}
}