#include "helpers/registerTypeHelper.hpp"
#include "data/ModelColliderComponent.hpp"

void registerModelColliderComponent() noexcept {
	kengine::registerComponents<
		kengine::ModelColliderComponent
	>();
}
