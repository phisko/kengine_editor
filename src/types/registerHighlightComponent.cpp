#include "helpers/registerTypeHelper.hpp"
#include "data/HighlightComponent.hpp"

void registerHighlightComponent() noexcept {
	kengine::registerComponents<
		kengine::HighlightComponent
	>();
}
