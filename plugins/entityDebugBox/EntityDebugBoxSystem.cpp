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
	putils::Point3f size{ 1.f, 1.f, 1.f };
} adjustables;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += functions::Execute{ execute };
				e += AdjustableComponent{ "Entity debug box", {
					{ "Active", &adjustables.active },
					{ "Color", &adjustables.boxColor },
					{ "Size X", &adjustables.size.x },
					{ "Size Y", &adjustables.size.y },
					{ "Size Z", &adjustables.size.z }
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

			for (auto [e, instance, debugGraphics] : entities.with<InstanceComponent, DebugGraphicsComponent>()) {
				auto & element = debugGraphics.elements[0];
				element.color = adjustables.boxColor;
				element.pos.y = adjustables.size.y / 2.f;

				auto & box = std::get<DebugGraphicsComponent::Box>(element.data);
				box.size = adjustables.size;
			}

			for (auto [e, instance, noDebugGraphics] : entities.with<InstanceComponent, no<DebugGraphicsComponent>>())
				e += DebugGraphicsComponent{ {
					DebugGraphicsComponent::Element{
						DebugGraphicsComponent::Box{ adjustables.size }, { 0.f, .5f, 0.f }, adjustables.boxColor, DebugGraphicsComponent::ReferenceSpace::World
					}
				} };
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
