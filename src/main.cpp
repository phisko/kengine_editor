#include <thread>

#include "go_to_bin_dir.hpp"
#include "PluginManager.hpp"
#include "kengine.hpp"

#include "helpers/mainLoop.hpp"
#include "helpers/imguiLuaHelper.hpp"

#include "systems/input/InputSystem.hpp"
#include "systems/lua/LuaSystem.hpp"
#include "systems/python/PythonSystem.hpp"
#include "systems/onclick/OnClickSystem.hpp"

#include "systems/imgui_adjustable/ImGuiAdjustableSystem.hpp"
#include "systems/imgui_engine_stats/ImGuiEngineStatsSystem.hpp"
#include "systems/imgui_tool/ImGuiToolSystem.hpp"
#include "systems/imgui_entity_editor/ImGuiEntityEditorSystem.hpp"
#include "systems/imgui_entity_selector/ImGuiEntitySelectorSystem.hpp"
#include "systems/imgui_prompt/ImGuiPromptSystem.hpp"

#include "systems/model_creator/ModelCreatorSystem.hpp"
#include "systems/polyvox/PolyVoxSystem.hpp"
#include "systems/polyvox/MagicaVoxelSystem.hpp"
#include "systems/assimp/AssimpSystem.hpp"
#include "systems/glfw/GLFWSystem.hpp"
#include "systems/opengl/OpenGLSystem.hpp"
#include "systems/opengl_sprites/OpenGLSpritesSystem.hpp"
#include "systems/recast/RecastSystem.hpp"

#include "systems/bullet/BulletSystem.hpp"
#include "systems/kinematic/KinematicSystem.hpp"

#include "data/WindowComponent.hpp"
#include "data/LuaComponent.hpp"
#include "data/PythonComponent.hpp"

int main(int, char **av) {
	putils::goToBinDir(av[0]);

#if defined(_WIN32) && defined(KENGINE_NDEBUG)
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif

	kengine::init(std::thread::hardware_concurrency());

	extern void registerTypes() noexcept;
	registerTypes();

	kengine::entities += [](kengine::Entity & e) noexcept {
		e += kengine::WindowComponent{
			"Kengine Editor"
		};
	};

	kengine::entities += kengine::InputSystem();
	kengine::entities += kengine::LuaSystem();
	kengine::entities += kengine::PythonSystem();
	
	kengine::entities += kengine::OnClickSystem();
	kengine::entities += kengine::ModelCreatorSystem();

	kengine::entities += kengine::OpenGLSystem();
	kengine::entities += kengine::GLFWSystem();
	kengine::entities += kengine::OpenGLSpritesSystem();
	kengine::entities += kengine::PolyVoxSystem();
	kengine::entities += kengine::MagicaVoxelSystem();
	kengine::entities += kengine::AssImpSystem();

	kengine::entities += kengine::BulletSystem();
	kengine::entities += kengine::KinematicSystem();
	kengine::entities += kengine::RecastSystem();

	kengine::entities += kengine::ImGuiAdjustableSystem();
	kengine::entities += kengine::ImGuiEngineStatsSystem();
	kengine::entities += kengine::ImGuiToolSystem();
	kengine::entities += kengine::ImGuiEntityEditorSystem();
	kengine::entities += kengine::ImGuiEntitySelectorSystem();
	kengine::entities += kengine::ImGuiPromptSystem();

	putils::PluginManager pm;
	pm.rescanDirectory("plugins", "loadKenginePlugin", kengine::getState());

	kengine::imguiLuaHelper::initBindings();

	kengine::mainLoop::timeModulated::run();

	kengine::terminate();

	return 0;
}
