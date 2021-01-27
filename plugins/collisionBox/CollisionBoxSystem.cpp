#include <GLFW/glfw3.h>
#include "kengine.hpp"
#include "Export.hpp"
#include "helpers/pluginHelper.hpp"

#include "data/EditorComponent.hpp"
#include "data/InstanceComponent.hpp"
#include "data/ModelColliderComponent.hpp"
#include "data/PhysicsComponent.hpp"
#include "data/TransformComponent.hpp"

#include "functions/DrawGizmos.hpp"
#include "functions/Execute.hpp"
#include "functions/OnClick.hpp"

#include "imgui.h"
#include "ImGuizmo.h"
#include "magic_enum.hpp"

namespace kengine::bullet {
	struct BulletPhysicsComponent;
}

using namespace kengine;

struct GizmoComponent {
	enum class Type {
		Translate,
		Scale,
		Rotate
	};

	Type type = Type::Translate;
	TransformComponent transform;
	TransformComponent modelTransformSave;
};

static bool g_active = true;
static int g_gizmoId = 0;
static bool g_shouldOpenContextMenu = false;
static GizmoComponent * g_gizmoForContextMenu = nullptr;
static std::optional<ModelColliderComponent::Collider::Shape> g_shapeToAdd = std::nullopt;

EXPORT void loadKenginePlugin(void * state) noexcept {
	struct impl {
		static void init() noexcept {
			entities += [](Entity & e) noexcept {
				e += EditorComponent{ "Collisions", &g_active };
				e += ::functions::DrawGizmos{ drawGizmos };
			};
		}

		static void drawGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			if (!g_active)
				return;

			processGizmos(proj, view, windowSize, windowPos);
			processContextMenu();
		}

		static void processGizmos(const glm::mat4 & proj, const glm::mat4 & view, const ImVec2 & windowSize, const ImVec2 & windowPos) noexcept {
			for (auto [e, instance] : entities.with<InstanceComponent>()) {
				auto model = entities[instance.model];
				auto & modelColliders = model.attach<ModelColliderComponent>();

				if (g_shapeToAdd) {
					modelColliders.colliders.push_back({ *g_shapeToAdd });
					g_shapeToAdd = std::nullopt;
				}

				e += PhysicsComponent{ 0.f };
			}
		}

		static void processContextMenu() noexcept {
			for (auto [e, instance, noOnClick] : entities.with<InstanceComponent, no<kengine::functions::OnClick>>()) {
				auto & gizmo = e.attach<GizmoComponent>();
				e += kengine::functions::OnClick{ [&gizmo](int button) {
					onClick(button, gizmo);
				} };
			}

			if (g_shouldOpenContextMenu) {
				ImGui::OpenPopup("Gizmo type##collisionBox");
				g_shouldOpenContextMenu = false;
			}

			if (ImGui::BeginPopup("Gizmo type##collisionBox")) {
				for (const auto [gizmoType, name] : putils::magic_enum::enum_entries<GizmoComponent::Type>())
					if (ImGui::MenuItem(putils::string<64>(name)))
						g_gizmoForContextMenu->type = gizmoType;

				if (ImGui::BeginMenu("Add collider")) {
					for (const auto [shape, name] : putils::magic_enum::enum_entries<ModelColliderComponent::Collider::Shape>())
						if (ImGui::MenuItem(putils::string<64>(name)))
							g_shapeToAdd = shape;
					ImGui::EndMenu();
				}

				ImGui::EndPopup();
			}
		}

		static void onClick(int button, GizmoComponent & gizmo) noexcept {
			if (button != GLFW_MOUSE_BUTTON_RIGHT)
				return;

			g_gizmoForContextMenu = &gizmo;
			g_shouldOpenContextMenu = true;
		}
	};

	pluginHelper::initPlugin(state);
	impl::init();
}
