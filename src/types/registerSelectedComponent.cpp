#include "helpers/registerTypeHelper.hpp"
#include "data/SelectedComponent.hpp"

void registerSelectedComponent() noexcept {
	kengine::registerComponents<
		kengine::SelectedComponent
	>();
}
