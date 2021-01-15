#include "helpers/registerTypeHelper.hpp"
#include "data/PhysicsComponent.hpp"
#include "data/KinematicComponent.hpp"

void registerPhysicsComponent() noexcept {
	kengine::registerComponents<
		kengine::PhysicsComponent,
		kengine::KinematicComponent
	>();
}
