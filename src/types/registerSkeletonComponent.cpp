#include "helpers/registerTypeHelper.hpp"
#include "data/ModelSkeletonComponent.hpp"

void registerSkeletonComponent() noexcept {
	kengine::registerComponents<
		// kengine::SkeletonComponent,
		kengine::ModelSkeletonComponent
	>();
}
