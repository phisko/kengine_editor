#include "helpers/registerTypeHelper.hpp"
#include "data/SpriteComponent.hpp"

void registerSpriteComponent() noexcept {
	kengine::registerComponents<
		kengine::SpriteComponent2D,
		kengine::SpriteComponent3D
	>();
}
