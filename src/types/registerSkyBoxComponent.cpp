#include "helpers/registerTypeHelper.hpp"
#include "data/SkyBoxComponent.hpp"

void registerSkyBoxComponent() noexcept {
	kengine::registerComponents<
		kengine::SkyBoxComponent
	>();
}
