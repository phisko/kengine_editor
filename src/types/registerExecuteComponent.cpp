#include "helpers/registerTypeHelper.hpp"
#include "functions/Execute.hpp"

void registerExecuteComponent() noexcept {
	kengine::registerComponents<
		kengine::functions::Execute
	>();
}
