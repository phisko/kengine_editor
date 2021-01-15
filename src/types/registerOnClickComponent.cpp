#include "helpers/registerTypeHelper.hpp"
#include "data/OnClickComponent.hpp"

void registerOnClickComponent() noexcept {
	kengine::registerComponents<
		kengine::OnClickComponent
	>();
}
