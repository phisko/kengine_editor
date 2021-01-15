#include "helpers/registerTypeHelper.hpp"
#include "data/ModelComponent.hpp"

void registerModelComponent() noexcept {
	kengine::registerComponents<
		kengine::ModelComponent
	>();
}
