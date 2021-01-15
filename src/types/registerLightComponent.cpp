#include "helpers/registerTypeHelper.hpp"
#include "data/LightComponent.hpp"

void registerLightComponent() noexcept {
	kengine::registerComponents<
		kengine::DirLightComponent,
		kengine::PointLightComponent,
		kengine::SpotLightComponent
	>();
}
