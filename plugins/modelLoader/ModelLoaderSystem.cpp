#include <filesystem>
#include <unordered_set>
#include <vector>
#include <fstream>

#include <GLFW/glfw3.h>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/GraphicsComponent.hpp"
#include "data/TransformComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ImGuiMainMenuBarItemComponent.hpp"
#include "data/GLFWWindowComponent.hpp"
#include "data/ModelComponent.hpp"
#include "data/SelectedComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/OnTerminate.hpp"

#include "meta/Has.hpp"
#include "meta/SaveToJSON.hpp"
#include "meta/ToSave.hpp"

#include "helpers/assertHelper.hpp"
#include "helpers/jsonHelper.hpp"
#include "helpers/sortHelper.hpp"
#include "helpers/typeHelper.hpp"

#include "imgui.h"
#include "imfilebrowser.h"

#include "magic_enum.hpp"
#include "concat.hpp"
#include "file_extension.hpp"

using namespace kengine;

static constexpr auto RECENT_FILE = "recentModels.txt";

static std::list<std::string> g_recentItems;
static ImGui::FileBrowser g_dialog(ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_EnterNewFilename);

enum class ModelAction {
	Load,
	Save
};
static ModelAction g_modelAction;

static EntityID g_currentEntity = INVALID_ID;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			loadRecentItems();

			typeHelper::getTypeEntity<ModelComponent>() += ::meta::ToSave{};

			entities += [](Entity & e) noexcept {
				e += functions::Execute{ execute };
				e += functions::OnTerminate{ saveRecentItems };
			};

			entities += [=](Entity & e) noexcept {
				e += ImGuiMainMenuBarItemComponent{ "File", "Model", [=]() noexcept {
					if (ImGui::BeginMenu("Model")) {
						for (const auto [action, name] : putils::magic_enum::enum_entries<ModelAction>()) {
							std::string actionName(name);
							if (ImGui::MenuItem(actionName.c_str())) {
								g_modelAction = action;
								g_dialog.SetTitle(std::move(actionName));
								g_dialog.Open();
							}
						}

						if (ImGui::BeginMenu("Recent")) {
							for (const auto & f : g_recentItems)
								if (ImGui::MenuItem(f.c_str()))
									loadModel(f.c_str());
							ImGui::EndMenu();
						}

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

		static void execute(float deltaTime) noexcept {
			g_dialog.Display();

			if (g_dialog.HasSelected()) {
				const auto selected = g_dialog.GetSelected().string();
				g_dialog.ClearSelected();

				switch (g_modelAction) {
				case ModelAction::Load:
					loadModel(selected.c_str());
					break;
				case ModelAction::Save:
					saveModel(selected.c_str());
					break;
				}
			}

			for (const auto & [e, window] : entities.with<GLFWWindowComponent>()) {
				glfwSetDropCallback(window.window.get(), [](GLFWwindow * window, int nbFiles, const char ** files) noexcept {
					kengine_assert(nbFiles >= 1);
					loadModel(files[0]);
				});
			}
		}

		static void loadModel(const char * path) noexcept {
			if (g_currentEntity != INVALID_ID) {
				const auto e = entities[g_currentEntity];
				if (e.has<InstanceComponent>())
					entities -= e.get<InstanceComponent>().model;
				else
					kengine_assert_failed("Entity does not have model");

				entities -= e;
			}

			if (putils::file_extension(path) == "json")
				loadFromJSON(path);
			else {
				entities += [&](Entity & e) noexcept {
					g_currentEntity = e.id;
					e += GraphicsComponent{ path };
					e += TransformComponent{};
					e += SelectedComponent{};
				};
			}

			const auto it = std::find(g_recentItems.begin(), g_recentItems.end(), path);
			if (it != g_recentItems.end())
				g_recentItems.splice(g_recentItems.begin(), g_recentItems, it);
			else
				g_recentItems.push_front(path);
		}

		static void loadFromJSON(const char * path) noexcept {
			std::ifstream f(path);
			if (!f) {
				kengine_assert_failed("Failed to open '", path, "'");
				return;
			}

			const auto modelJSON = putils::json::parse(f);
			const auto model = jsonHelper::createEntity(modelJSON);
			entities += [&](Entity & e) noexcept {
				g_currentEntity = e.id;
				e += InstanceComponent{ model.id };
				e += GraphicsComponent{};
				e += TransformComponent{};
				e += SelectedComponent{};
			};
		}

		static void saveModel(const char * path) noexcept {
			if (g_currentEntity == INVALID_ID)
				return;

			std::ofstream f(path);
			if (!f) {
				kengine_assert_failed("Could not open '", path, "' for writing");
				return;
			}

			const auto e = entities[g_currentEntity];
			const auto instance = e.tryGet<InstanceComponent>();
			if (!instance) {
				kengine_assert_failed("Entity does not have model");
				return;
			}
			const auto model = entities[instance->model];
			const auto json = saveToJSON(model);
			f << json.dump(4);
		}

		static putils::json saveToJSON(const Entity & e) noexcept {
			putils::json ret;

			const auto types = sortHelper::getNameSortedEntities<KENGINE_COMPONENT_COUNT,
				kengine::meta::Has, kengine::meta::SaveToJSON, ::meta::ToSave
			>();

			for (const auto & [_, name, has, save, toSave] : types) {
				if (!has->call(e))
					continue;
				ret[name->name.c_str()] = save->call(e);
			}

			return ret;
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}