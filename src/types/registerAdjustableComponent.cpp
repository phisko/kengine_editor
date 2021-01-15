#include "helpers/registerTypeHelper.hpp"
#include "data/AdjustableComponent.hpp"

void registerAdjustableComponent() noexcept {
	kengine::registerComponents<
		kengine::AdjustableComponent
	>();
}
