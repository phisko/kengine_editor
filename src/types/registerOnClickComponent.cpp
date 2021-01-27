#include "helpers/registerTypeHelper.hpp"
#include "functions/OnClick.hpp"

void registerOnClickComponent() noexcept {
	kengine::registerComponents<
		kengine::functions::OnClick
	>();
}
