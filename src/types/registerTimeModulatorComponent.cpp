#include "helpers/registerTypeHelper.hpp"
#include "data/TimeModulatorComponent.hpp"

void registerTimeModulatorComponent() noexcept {
	kengine::registerComponents<
		kengine::TimeModulatorComponent
	>();
}
