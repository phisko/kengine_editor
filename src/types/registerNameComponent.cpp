#include "helpers/registerTypeHelper.hpp"
#include "data/NameComponent.hpp"

void registerNameComponent() noexcept {
	kengine::registerComponents<
		kengine::NameComponent
	>();
}
