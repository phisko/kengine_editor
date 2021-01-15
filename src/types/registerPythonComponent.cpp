#include "helpers/registerTypeHelper.hpp"
#include "data/PythonComponent.hpp"

void registerPythonComponent() noexcept {
	kengine::registerComponents<
		kengine::PythonComponent
	>();
}
