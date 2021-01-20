#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/DebugGraphicsComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "functions/Execute.hpp"

static struct {
	bool active = true;
	putils::NormalizedColor boxColor{ 1.f, 1.f, 1.f, .25f };
} adjustables;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += functions::Execute{ execute };
				e += AdjustableComponent{ "Entity debug box", {
					{ "Active", &adjustables.active },
					{ "Color", &adjustables.boxColor }
				} };
			};
		}

		static void execute(float deltaTime) noexcept {
			// Need to check for InstanceComponent as well since BulletSystem has a debug entity
			if (!adjustables.active) {
				for (auto [e, instance, debugGraphics] : entities.with<InstanceComponent, DebugGraphicsComponent>())
					e.detach<DebugGraphicsComponent>();
				return;
			}

			for (auto [e, instance, debugGraphics] : entities.with<InstanceComponent, DebugGraphicsComponent>())
				debugGraphics.elements[0].color = adjustables.boxColor;

			for (auto [e, instance, noDebugGraphics] : entities.with<InstanceComponent, no<DebugGraphicsComponent>>())
				e += DebugGraphicsComponent{ {
					DebugGraphicsComponent::Element{
						DebugGraphicsComponent::Box{}, { 0.f, .5f, 0.f }, adjustables.boxColor, DebugGraphicsComponent::ReferenceSpace::World
					}
				} };
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
