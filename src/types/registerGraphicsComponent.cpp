#include "helpers/registerTypeHelper.hpp"
#include "data/GraphicsComponent.hpp"

void registerGraphicsComponent() noexcept {
	kengine::registerComponents<
		kengine::GraphicsComponent
	>();
}
