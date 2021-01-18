#include <filesystem>
#include <unordered_set>
#include <vector>
#include <fstream>

#include <GLFW/glfw3.h>

#include "Export.hpp"
#include "kengine.hpp"

#include "data/GraphicsComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "data/GLFWWindowComponent.hpp"
#include "data/SelectedComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/OnTerminate.hpp"

#include "helpers/pluginHelper.hpp"
#include "helpers/assertHelper.hpp"

#include "imgui.h"
#include "imfilebrowser.h"

static constexpr auto RECENT_FILE = "recentModels.txt";

static std::list<std::string> g_recentItems;
static ImGui::FileBrowser g_dialog;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			loadRecentItems();

			entities += [](Entity & e) noexcept {
				g_dialog.SetTitle("Load model");

				e += functions::Execute{ execute };
				e += functions::OnTerminate{ saveRecentItems };
			};

			entities += [](Entity & e) noexcept {
				e += ImGuiMainMenuBarItemComponent{ "File", "Load model", []() noexcept {
					if (ImGui::MenuItem("Load model"))
						g_dialog.Open();
				} };
			};

			entities += [](Entity & e) noexcept {
				e += ImGuiMainMenuBarItemComponent{ "File", "Recent models", []() noexcept {
					if (ImGui::BeginMenu("Recent models")) {
						for (const auto & f : g_recentItems)
							if (ImGui::MenuItem(f.c_str()))
								loadModel(f.c_str());
						ImGui::EndMenu();
					}
				} };
			};
		}

		static void loadRecentItems() noexcept {
			std::ifstream f(RECENT_FILE);

			if (!f)
				return;

			for (std::string s; std::getline(f, s);)
				g_recentItems.push_back(s);
		}

		static void saveRecentItems() noexcept {
			std::ofstream f(RECENT_FILE, std::ofstream::trunc);

			if (!f)
				return;

			for (const auto & s : g_recentItems)
				f << s << std::endl;
		}

		static void loadModel(const char * path) noexcept {
			static EntityID toRemove = INVALID_ID;

			if (toRemove != INVALID_ID) {
				const auto & e = entities[toRemove];
				if (e.has<InstanceComponent>())
					entities -= e.get<InstanceComponent>().model;

				entities -= toRemove;
			}

			entities += [&](Entity & e) noexcept {
				toRemove = e.id;
				e += GraphicsComponent{ path };
				e += TransformComponent{};
				e += SelectedComponent{};
			};

			const auto it = std::find(g_recentItems.begin(), g_recentItems.end(), path);
			if (it != g_recentItems.end())
				g_recentItems.splice(g_recentItems.begin(), g_recentItems, it);
			else
				g_recentItems.push_front(path);
		}

		static void execute(float deltaTime) noexcept {
			g_dialog.Display();

			if (g_dialog.HasSelected()) {
				loadModel(g_dialog.GetSelected().string().c_str());
				g_dialog.ClearSelected();
			}

			for (const auto & [e, window] : entities.with<GLFWWindowComponent>()) {
				glfwSetDropCallback(window.window.get(), [](GLFWwindow * window, int nbFiles, const char ** files) noexcept {
					kengine_assert(nbFiles >= 1);
					loadModel(files[0]);
				});
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}