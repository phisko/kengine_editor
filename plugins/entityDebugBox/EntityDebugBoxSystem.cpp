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

			const auto setElementProperties = [](DebugGraphicsComponent::Element & element) noexcept {
				element.color = adjustables.boxColor;
				element.pos.y = adjustables.size.y / 2.f;
				element.box.size = adjustables.size;
			};

			for (auto [e, instance, debugGraphics] : entities.with<InstanceComponent, DebugGraphicsComponent>()) {
				auto & element = debugGraphics.elements[0];
				setElementProperties(element);
			}

			for (auto [e, instance, noDebugGraphics] : entities.with<InstanceComponent, no<DebugGraphicsComponent>>()) {
				DebugGraphicsComponent::Element debug; {
					debug.type = DebugGraphicsComponent::Type::Box;
					debug.color = adjustables.boxColor;
					setElementProperties(debug);
				}
				e += DebugGraphicsComponent{ { std::move(debug) } };
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
