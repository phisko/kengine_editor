#include "helpers/registerTypeHelper.hpp"
#include "data/NavMeshComponent.hpp"

void registerNavMeshComponent() noexcept {
	kengine::registerComponents<
		kengine::NavMeshComponent,
		kengine::RebuildNavMeshComponent
	>();
}
