#include <GLFW/glfw3.h>

#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/HighlightComponent.hpp"
#include "data/SelectedComponent.hpp"
#include "data/SpriteComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/GetEntityInPixel.hpp"

struct HoveredComponent {};

static putils::NormalizedColor SELECTED_COLOR;
static float SELECTED_INTENSITY = 2.f;

static putils::NormalizedColor HOVERED_COLOR;
static float HOVERED_INTENSITY = 1.f;

using namespace kengine;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += functions::Execute{ execute };

				e += AdjustableComponent{
					"Highlight", {
						{ "Selected color", &SELECTED_COLOR },
						{ "Selected intensity", &SELECTED_INTENSITY },
						{ "Hovered color", &HOVERED_COLOR },
						{ "Hovered intensity", &HOVERED_INTENSITY }
					}
				};

				InputComponent input;
				input.onMouseButton = [](EntityID window, int button, const putils::Point2f & coords, bool pressed) noexcept {
					if (!pressed || button != GLFW_MOUSE_BUTTON_LEFT)
						return;
					click(window, coords);
				};
				input.onMouseMove = [](EntityID window, const putils::Point2f & coords, const putils::Point2f & rel) noexcept {
					hover(window, coords);
				};

				e += input;
			};
		}

		static void execute(float deltaTime) noexcept {
			for (auto [e, highlight, noSelected, noHovered] : entities.with<HighlightComponent, no<SelectedComponent>, no<HoveredComponent>>())
				e.detach<HighlightComponent>();

			for (auto [e, selected, notHighlighted] : entities.with<SelectedComponent, no<HighlightComponent>>())
				e += HighlightComponent{ .color = SELECTED_COLOR, .intensity = SELECTED_INTENSITY };
		}

		static void click(EntityID window, const putils::Point2f & coords) noexcept {
			EntityID id = INVALID_ID;
			for (const auto & [e, func] : entities.with<functions::GetEntityInPixel>()) {
				id = func(window, coords);
				if (id != INVALID_ID)
					break;
			}

			if (id == INVALID_ID)
				return;

			auto e = entities[id];
			if (e.has<SelectedComponent>())
				e.detach<SelectedComponent>();
			else {
				e += SelectedComponent{};
				e += HighlightComponent{ .color = SELECTED_COLOR,.intensity = SELECTED_INTENSITY };
			}
		}

		static void hover(EntityID window, const putils::Point2f & coords) noexcept {
			static EntityID previous = INVALID_ID;

			EntityID hovered = INVALID_ID;
			for (const auto & [e, func] : entities.with<functions::GetEntityInPixel>()) {
				hovered = func(window, coords);
				if (hovered != INVALID_ID)
					break;
			}

			if (hovered == previous)
				return;

			if (previous != INVALID_ID) {
				auto e = entities[previous];
				if (e.has<HoveredComponent>())
					e.detach<HoveredComponent>();

				previous = INVALID_ID;
			}

			if (hovered != INVALID_ID) {
				auto e = entities[hovered];
				if (!e.has<SelectedComponent>()) {
					e += HoveredComponent{};
					e += HighlightComponent{ .color = HOVERED_COLOR,.intensity = HOVERED_INTENSITY };
				}

				previous = hovered;
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
