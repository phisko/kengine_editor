#include "helpers/registerTypeHelper.hpp"
#include "data/InputComponent.hpp"

void registerInputComponent() noexcept {
	kengine::registerComponents<
		kengine::InputComponent
	>();

	kengine::registerTypes<
		putils::Point2f
	>();
}
