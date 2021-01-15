#include "helpers/registerTypeHelper.hpp"
#include "data/ImGuiToolComponent.hpp"

void registerImGuiComponent() noexcept {
	kengine::registerComponents<
		kengine::ImGuiToolComponent
	>();
}
