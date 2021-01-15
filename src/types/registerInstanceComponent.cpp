#include "helpers/registerTypeHelper.hpp"
#include "data/InstanceComponent.hpp"

void registerInstanceComponent() noexcept {
	kengine::registerComponents<
		kengine::InstanceComponent
	>();
}
