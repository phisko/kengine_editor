#include <GLFW/glfw3.h>
#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/AdjustableComponent.hpp"
#include "data/DebugGraphicsComponent.hpp"
#include "data/EditorComponent.hpp"
#include "data/KinematicComponent.hpp"
#include "data/InputComponent.hpp"
#include "data/NavMeshComponent.hpp"
#include "data/PathfindingComponent.hpp"
#include "data/PhysicsComponent.hpp"
#include "data/TransformComponent.hpp"

#include "functions/Execute.hpp"
#include "functions/GetPositionInPixel.hpp"

#include "meta/ToSave.hpp"

#include "helpers/instanceHelper.hpp"
#include "helpers/typeHelper.hpp"

#include "imgui.h"
#include "helpers/imfilebrowser.h"
#include "reflection/imgui_helper.hpp"

using namespace kengine;

struct {
	float speed = 1.f;
} adjustables;

static bool g_active = true;
static EntityID g_actor = INVALID_ID;
static std::optional<putils::Point3f> g_clickedPosition = std::nullopt;
static bool g_openContextMenu = false;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			typeHelper::getTypeEntity<NavMeshComponent>() += meta::ToSave{};

			entities += [](Entity & e) noexcept {
				e += EditorComponent{ "Navmesh", &g_active };
				e += functions::Execute{ execute };
				e += InputComponent{ .onMouseButton = onClick };
				e += AdjustableComponent{
					"Navmesh", {
						{ "Debug movement speed", &adjustables.speed }
					}
				};
			};
		}

		static void execute(float deltaTime) noexcept {
			handleActivationChange();
			handleContextMenu();
			if (!g_active)
				return;

			for (const auto [e, instance] : entities.with<InstanceComponent>()) {
				auto model = entities[instance.model];
				auto & navMesh = model.attach<NavMeshComponent>();

				if (ImGui::Begin("Navmesh"))
					putils::reflection::imguiEdit(navMesh);
				ImGui::End();

				auto actor = entities[g_actor];
				auto & pathfinding = actor.get<PathfindingComponent>();
				pathfinding.environment = e.id;
				pathfinding.maxSpeed = adjustables.speed;

				auto & transform = actor.get<TransformComponent>();
				transform.boundingBox.size.x = navMesh.characterRadius * 2.f;
				transform.boundingBox.size.y = navMesh.characterHeight;
				transform.boundingBox.size.z = navMesh.characterRadius * 2.f;

				auto & debug = actor.get<DebugGraphicsComponent>();
				debug.elements[0].pos.y = transform.boundingBox.size.y / 2.f;
			}
		}

		static void handleActivationChange() noexcept {
			static std::optional<bool> g_previousActive = std::nullopt;
			const auto activeChanged = !g_previousActive || (*g_previousActive != g_active);
			if (!activeChanged)
				return;

			g_previousActive = g_active;

			if (g_active) {
				entities += [](Entity & e) noexcept {
					g_actor = e.id;
					e += TransformComponent{};
					DebugGraphicsComponent::Element debug;
					debug.type = DebugGraphicsComponent::Type::Box;
					debug.color = { 1.f, 0.f, 0.f, 1.f };
					e += DebugGraphicsComponent{ { std::move(debug) } };
					e += PathfindingComponent{};
					e += KinematicComponent{};
					e += PhysicsComponent{};
				};
			}
			else
				entities -= g_actor;
		}

		static void handleContextMenu() noexcept {
			if (!g_clickedPosition)
				return;

			if (g_openContextMenu) {
				ImGui::OpenPopup("Navmesh actor");
				g_openContextMenu = false;
			}

			if (ImGui::BeginPopup("Navmesh actor")) {
				auto actor = entities[g_actor];

				if (ImGui::MenuItem("Teleport")) {
					actor.get<TransformComponent>().boundingBox.position = *g_clickedPosition;
					g_clickedPosition = std::nullopt;
				}

				if (ImGui::MenuItem("Navigate")) {
					actor.get<PathfindingComponent>().destination = *g_clickedPosition;
					g_clickedPosition = std::nullopt;
				}

				ImGui::EndPopup();
			}
		}

		static void onClick(EntityID window, int button, const putils::Point2f & screenCoordinates, bool pressed) noexcept {
			if (button != GLFW_MOUSE_BUTTON_RIGHT)
				return;

			for (const auto & [e, getPosition] : entities.with<functions::GetPositionInPixel>()) {
				const auto pos = getPosition(window, screenCoordinates);
				if (!pos)
					continue;

				g_clickedPosition = pos;
				g_openContextMenu = true;
			}
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
