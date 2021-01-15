#include "helpers/registerTypeHelper.hpp"
#include "data/GodRaysComponent.hpp"

void registerGodRaysComponent() noexcept {
	kengine::registerComponents<
		kengine::GodRaysComponent
	>();
}
