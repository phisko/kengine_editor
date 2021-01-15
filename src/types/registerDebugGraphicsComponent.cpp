#include "helpers/registerTypeHelper.hpp"
#include "data/DebugGraphicsComponent.hpp"

void registerDebugGraphicsComponent() noexcept {
	kengine::registerComponents<
		kengine::DebugGraphicsComponent
	>();
}
