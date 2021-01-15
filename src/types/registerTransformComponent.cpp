#include "helpers/registerTypeHelper.hpp"
#include "data/TransformComponent.hpp"

void registerTransformComponent() noexcept {
	kengine::registerComponents<
		kengine::TransformComponent
	>();
}
