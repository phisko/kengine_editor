#include "helpers/registerTypeHelper.hpp"
#include "functions/AppearsInViewport.hpp"

void registerAppearsInViewportComponent() noexcept {
	kengine::registerComponents<
		kengine::functions::AppearsInViewport
	>();
}
