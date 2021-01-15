#include "helpers/registerTypeHelper.hpp"
#include "data/PathfindingComponent.hpp"

void registerPathfindingComponent() noexcept {
	kengine::registerComponents<
		kengine::PathfindingComponent
	>();
}
